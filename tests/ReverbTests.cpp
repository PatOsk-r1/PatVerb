#include "engine/Reverb.h"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <random>
#include <vector>

using namespace pv;

namespace {

struct Rig {
    std::unique_ptr<Reverb> rv = std::make_unique<Reverb>();
    std::vector<float> params;
    float sr = 48000;
    Rig() {
        for (int i = 0; i < kNumParams; ++i) params.push_back(info(i).def);
        rv->prepare(sr, 512);
    }
    float& p(P id) { return params[static_cast<std::size_t>(id)]; }
    // Processes `in` (mono into both channels); returns the left output.
    std::vector<float> run(const std::vector<float>& in, std::vector<float>* right = nullptr) {
        std::vector<float> l = in, r = in;
        for (std::size_t i = 0; i < in.size(); i += 512) {
            const int n = static_cast<int>(std::min<std::size_t>(512, in.size() - i));
            rv->setParams(params.data(), 120.0);
            rv->process(l.data() + i, r.data() + i, n);
        }
        if (right) *right = r;
        return l;
    }
};

double rms(const std::vector<float>& x, double from, double to, float sr) {
    const auto a = static_cast<std::size_t>(from * sr), b = std::min(x.size(), static_cast<std::size_t>(to * sr));
    double s = 0;
    for (std::size_t i = a; i < b; ++i) s += x[i] * x[i];
    return std::sqrt(s / static_cast<double>(std::max<std::size_t>(1, b - a)));
}

}  // namespace

TEST(Reverb, DryWhenMixIsZero) {
    Rig rig;
    rig.p(P::Mix) = 0;
    std::vector<float> in(48000);
    for (std::size_t i = 0; i < in.size(); ++i) in[i] = std::sin(static_cast<float>(i) * 0.05f) * 0.5f;
    const auto out = rig.run(in);
    for (std::size_t i = 1000; i < in.size(); ++i) ASSERT_NEAR(out[i], in[i], 1e-4f);
}

TEST(Reverb, DecayTimeMatchesTheKnob) {
    // Schroeder backward integration of the impulse response; T20 extrapolated to 60 dB.
    for (Mode mode : {Mode::Hall, Mode::Room, Mode::Plate, Mode::Cloud}) {
        Rig rig;
        rig.p(P::Mode) = static_cast<float>(mode);
        rig.p(P::Mix) = 1;
        rig.p(P::Early) = 0;
        rig.p(P::Decay) = 2.0f;
        rig.p(P::Damping) = 20000;
        rig.p(P::BassCut) = 20;
        rig.p(P::PreDelay) = 0;
        std::vector<float> in(static_cast<std::size_t>(48000 * 5), 0.0f);
        in[0] = 1.0f;
        std::vector<float> r;
        const auto l = rig.run(in, &r);
        std::vector<double> edc(l.size() + 1, 0.0);
        for (std::size_t i = l.size(); i-- > 0;) edc[i] = edc[i + 1] + l[i] * l[i] + r[i] * r[i];
        const auto timeAt = [&](double db) {
            for (std::size_t i = 0; i < l.size(); ++i)
                if (10.0 * std::log10(edc[i] / edc[0]) <= db) return static_cast<double>(i) / 48000.0;
            return 99.0;
        };
        const double t60 = (timeAt(-25.0) - timeAt(-5.0)) * 3.0;
        EXPECT_NEAR(t60, 2.0, 0.7) << "mode " << static_cast<int>(mode);
    }
}

TEST(Reverb, EveryModeStaysStableAtExtremeSettings) {
    for (int mode = 0; mode < 7; ++mode) {
        Rig rig;
        rig.p(P::Mode) = static_cast<float>(mode);
        rig.p(P::Mix) = 1;
        rig.p(P::Decay) = 30;
        rig.p(P::Size) = 1;
        rig.p(P::Diffusion) = 1;
        rig.p(P::ModDepth) = 1;
        rig.p(P::ModRate) = 5;
        rig.p(P::ShimAmount) = 1;
        std::mt19937 rng(3);
        std::uniform_real_distribution<float> u(-0.5f, 0.5f);
        std::vector<float> in(static_cast<std::size_t>(48000 * 8));
        for (std::size_t i = 0; i < 48000 * 2; ++i) in[i] = u(rng);
        const auto out = rig.run(in);
        float peak = 0;
        for (float v : out) {
            ASSERT_TRUE(std::isfinite(v)) << mode;
            peak = std::max(peak, std::abs(v));
        }
        EXPECT_LT(peak, 6.0f) << mode;
        EXPECT_GT(rms(out, 3.0, 4.0, 48000), 1e-4) << mode;  // a 30 s tail is still ringing
    }
}

TEST(Reverb, FreezeHoldsTheTail) {
    Rig rig;
    rig.p(P::Mix) = 1;
    rig.p(P::Decay) = 1.0f;
    std::vector<float> in(12000, 0.0f);  // 0.1 s of sound, then freeze 0.15 s later
    for (int i = 0; i < 4800; ++i) in[static_cast<std::size_t>(i)] = std::sin(static_cast<float>(i) * 0.07f) * 0.5f;
    rig.run(in);
    rig.p(P::Freeze) = 1;
    std::vector<float> silence(48000 * 4, 0.0f);
    for (int i = 0; i < 4800; ++i) silence[static_cast<std::size_t>(i)] = 0.3f;  // ignored while frozen
    const auto out = rig.run(silence);
    const double early = rms(out, 0.5, 1.0, 48000), late = rms(out, 3.5, 4.0, 48000);
    EXPECT_GT(early, 1e-3);
    EXPECT_GT(late / early, 0.5);  // a 1 s reverb would be 100+ dB down by now if not frozen
}

TEST(Reverb, PreDelayHoldsBackTheWetSignal) {
    Rig rig;
    rig.p(P::Mix) = 1;
    rig.p(P::PreDelay) = 100;
    std::vector<float> in(48000, 0.0f);
    in[0] = 1.0f;
    const auto out = rig.run(in);
    std::size_t first = 0;
    while (first < out.size() && std::abs(out[first]) < 1e-3f) ++first;
    EXPECT_GE(first, 4800u - 10);
    EXPECT_LT(first, 4800u + 48 * 10);
}

TEST(Reverb, DuckingLowersTheTailWhilePlaying) {
    std::vector<float> in(48000 * 2);
    for (std::size_t i = 0; i < in.size(); ++i) in[i] = std::sin(static_cast<float>(i) * 0.03f) * 0.5f;
    Rig a, b;
    a.p(P::Mix) = b.p(P::Mix) = 1;
    b.p(P::Duck) = 1;
    const auto oa = a.run(in), ob = b.run(in);
    EXPECT_LT(rms(ob, 1.0, 2.0, 48000), rms(oa, 1.0, 2.0, 48000) * 0.5);
}

TEST(Reverb, CountsHitsForTheVisuals) {
    Rig rig;
    std::vector<float> in(48000 * 2, 0.0f);
    for (int hit = 0; hit < 4; ++hit)
        for (int i = 0; i < 2000; ++i) in[static_cast<std::size_t>(hit * 24000 + i)] = std::sin(static_cast<float>(i) * 0.1f) * 0.8f * std::exp(-i / 500.0f);
    rig.run(in);
    EXPECT_EQ(rig.rv->meters().onsets, 4);
}
