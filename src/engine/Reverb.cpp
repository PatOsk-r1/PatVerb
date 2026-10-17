#include "engine/Reverb.h"

namespace pv {
namespace {

inline float val(const std::array<float, kNumParams>& p, P id) noexcept { return p[static_cast<std::size_t>(id)]; }

// Late-reverb line lengths (ms) per mode, mutually prime-ish.
constexpr float kHall[8] = {37.1f, 41.3f, 47.9f, 53.3f, 59.1f, 67.3f, 73.9f, 83.1f};
constexpr float kRoom[8] = {11.3f, 13.7f, 15.1f, 17.9f, 19.3f, 21.7f, 23.9f, 27.1f};
constexpr float kChamber[8] = {19.1f, 22.7f, 25.3f, 28.9f, 31.3f, 34.7f, 37.1f, 41.9f};
constexpr float kDiff[8] = {4.77f, 3.59f, 12.73f, 9.31f, 7.21f, 5.43f, 15.9f, 11.3f};
// Early reflection taps (ms, gain) for each side.
constexpr float kErL[12] = {3.1f, 7.3f, 11.9f, 17.2f, 23.4f, 29.9f, 37.1f, 44.8f, 53.2f, 62.0f, 71.3f, 81.1f};
constexpr float kErR[12] = {4.3f, 8.9f, 13.1f, 19.6f, 25.7f, 32.3f, 39.8f, 47.5f, 55.9f, 65.1f, 74.6f, 84.9f};
// Dattorro plate, in samples at 29761 Hz.
constexpr float kPlateRate = 29761.0f;

}  // namespace

void Reverb::prepare(double sampleRate, int /*maxBlock*/) {
    sr_ = static_cast<float>(sampleRate);
    const auto ms = [this](float m) { return static_cast<int>(sr_ * m * 0.001f) + 8; };
    preL_.allocate(ms(2100));
    preR_.allocate(ms(2100));
    erLine_.allocate(ms(220));
    for (auto& l : lines_) l.allocate(ms(260));
    for (auto& a : diffL_) a.line.allocate(ms(40));
    for (auto& a : diffR_) a.line.allocate(ms(40));
    shimLine_.allocate(ms(120));
    const float k = sr_ / kPlateRate * 1.6f;
    for (auto& a : plateIn_) a.line.allocate(static_cast<int>(400 * k));
    apA1_.line.allocate(static_cast<int>(720 * k));
    apB1_.line.allocate(static_cast<int>(960 * k));
    apA2_.line.allocate(static_cast<int>(1850 * k));
    apB2_.line.allocate(static_cast<int>(2700 * k));
    dA1_.allocate(static_cast<int>(4500 * k));
    dA2_.allocate(static_cast<int>(3760 * k));
    dB1_.allocate(static_cast<int>(4260 * k));
    dB2_.allocate(static_cast<int>(3200 * k));
    for (auto& s : springAp_)
        for (auto& a : s) a.line.allocate(16);
    for (auto& l : springLine_) l.allocate(ms(80));
    for (int i = 0; i < kNumParams; ++i) p_[static_cast<std::size_t>(i)] = info(i).def;
    reset();
}

void Reverb::reset() noexcept {
    preL_.clear();
    preR_.clear();
    erLine_.clear();
    for (auto& l : lines_) l.clear();
    for (auto& a : diffL_) a.line.clear();
    for (auto& a : diffR_) a.line.clear();
    shimLine_.clear();
    for (auto& a : plateIn_) a.line.clear();
    for (auto* a : {&apA1_, &apA2_, &apB1_, &apB2_}) a->line.clear();
    for (auto* d : {&dA1_, &dA2_, &dB1_, &dB2_}) d->clear();
    for (auto& s : springAp_)
        for (auto& a : s) a.line.clear();
    for (auto& l : springLine_) l.clear();
    for (auto& f : damp_) f.z = 0;
    for (auto& f : bass_) f.z = 0;
    springFb_.fill(0);
    plateFbA_ = plateFbB_ = 0;
    for (auto* b : {&lowCutL_, &lowCutR_, &highCutL_, &highCutR_}) b->reset();
    duckEnv_ = 0;
    for (int k = 0; k < kLines; ++k) phase_[static_cast<std::size_t>(k)] = static_cast<float>(k) * 0.785f;
}

void Reverb::setParams(const float* plain, double bpm) noexcept {
    for (int i = 0; i < kNumParams; ++i) p_[static_cast<std::size_t>(i)] = plain[i];
    bpm_ = bpm > 0 ? bpm : 120.0;
    const Mode m = static_cast<Mode>(std::clamp(static_cast<int>(std::lround(val(p_, P::Mode))), 0, 6));
    if (m != mode_) {  // a different algorithm starts from silence instead of garbage
        mode_ = m;
        reset();
    }
    freeze_ = val(p_, P::Freeze) > 0.5f;
    dampA_ = OnePole::coef(val(p_, P::Damping), sr_);
    bassA_ = OnePole::coef(val(p_, P::BassCut), sr_);
    duckRel_ = std::exp(-1.0f / (val(p_, P::DuckRelease) * 0.001f * sr_));
    shimRatio_ = std::exp2(shimmerSemitones(static_cast<int>(std::lround(val(p_, P::ShimPitch)))) / 12.0f);
    if (val(p_, P::LowCut) != lastLowCut_) {
        lastLowCut_ = val(p_, P::LowCut);
        lowCutL_.highpass(sr_, lastLowCut_);
        lowCutR_.highpass(sr_, lastLowCut_);
    }
    if (val(p_, P::HighCut) != lastHighCut_) {
        lastHighCut_ = val(p_, P::HighCut);
        highCutL_.lowpass(sr_, lastHighCut_);
        highCutR_.lowpass(sr_, lastHighCut_);
    }
    updateLengths();
}

void Reverb::updateLengths() noexcept {
    // Size glides between blocks so turning the knob does not click.
    size_.coef = 0.15f;
    const float size = size_.next(val(p_, P::Size));
    const float decay = std::max(0.1f, val(p_, P::Decay));
    const float diffusion = val(p_, P::Diffusion);

    const float* base = kHall;
    float scale = 0.45f + size * 1.35f, modScale = 1.0f;
    numDiffusers_ = 4;
    switch (mode_) {
        case Mode::Room: base = kRoom; scale = 0.5f + size * 1.0f; modScale = 0.5f; break;
        case Mode::Chamber: base = kChamber; scale = 0.5f + size * 1.1f; modScale = 0.7f; break;
        case Mode::Cloud: scale = (0.45f + size * 1.35f) * 1.25f; modScale = 3.0f; numDiffusers_ = 8; break;
        default: break;
    }
    for (int k = 0; k < kLines; ++k) {
        const auto sk = static_cast<std::size_t>(k);
        len_[sk] = std::min(base[k] * scale * sr_ * 0.001f, static_cast<float>(lines_[sk].capacity()) - 64.0f);
        gain_[sk] = freeze_ ? 1.0f : std::pow(10.0f, -3.0f * len_[sk] / (decay * sr_));
    }
    for (int i = 0; i < 8; ++i) diffLen_[static_cast<std::size_t>(i)] = kDiff[i] * (0.5f + size * 0.8f) * sr_ * 0.001f;
    diffG_ = (mode_ == Mode::Cloud ? 0.55f : 0.45f) + 0.27f * diffusion;
    modDepth_ = val(p_, P::ModDepth) * sr_ * 0.0035f * modScale;

    plateScale_ = sr_ / kPlateRate * (0.5f + size);
    const float plateHalf = (672.0f + 4453.0f + 1800.0f + 3720.0f) * plateScale_;
    // The decay gain is applied twice per tank half (entering the half and after its first delay).
    plateDecay_ = freeze_ ? 1.0f : std::min(0.9995f, std::pow(10.0f, -3.0f * 0.5f * plateHalf / (decay * sr_)));

    for (int c = 0; c < 2; ++c) {
        springLen_[static_cast<std::size_t>(c)] = (25.0f + size * 40.0f) * (c ? 1.071f : 1.0f) * sr_ * 0.001f;
    }
    springGain_ = freeze_ ? 1.0f : std::pow(10.0f, -3.0f * springLen_[0] / (decay * sr_));
}

float Reverb::shimmer(float x) noexcept {
    // Two crossfaded taps sweeping a 60 ms window: a simple granular pitch shifter.
    shimLine_.write(x);
    const float window = 0.06f * sr_;
    shimPhase_ += (1.0f - shimRatio_) / window;
    shimPhase_ -= std::floor(shimPhase_);
    const float p2 = shimPhase_ + 0.5f - std::floor(shimPhase_ + 0.5f);
    const float g1 = std::sin(kPi * shimPhase_), g2 = std::sin(kPi * p2);
    return shimLine_.read(2.0f + shimPhase_ * window) * g1 * g1 + shimLine_.read(2.0f + p2 * window) * g2 * g2;
}

void Reverb::processFdn(float inL, float inR, float& outL, float& outR) noexcept {
    for (int i = 0; i < numDiffusers_; ++i) {
        const auto si = static_cast<std::size_t>(i);
        inL = diffL_[si].process(inL, diffLen_[si], diffG_);
        inR = diffR_[si].process(inR, diffLen_[si] * 1.13f, diffG_);
    }
    const float rate = val(p_, P::ModRate);
    std::array<float, kLines> y{}, s{};
    for (int k = 0; k < kLines; ++k) {
        const auto sk = static_cast<std::size_t>(k);
        phase_[sk] += kTwoPi * rate * (1.0f + 0.13f * static_cast<float>(k)) / sr_;
        if (phase_[sk] > kTwoPi) phase_[sk] -= kTwoPi;
        const float d = len_[sk] + modDepth_ * (0.5f + 0.5f * std::sin(phase_[sk])) + 2.0f;
        float v = lines_[sk].read(d);
        y[sk] = v;
        if (!freeze_) {
            v = damp_[sk].lp(v, dampA_);
            v -= bass_[sk].lp(v, bassA_);
        }
        s[sk] = v * gain_[sk];
    }
    // Fast Walsh-Hadamard transform: orthogonal mixing keeps the energy (freeze stays frozen).
    for (int h = 1; h < kLines; h <<= 1)
        for (int a = 0; a < kLines; a += h << 1)
            for (int b = a; b < a + h; ++b) {
                const float p = s[static_cast<std::size_t>(b)], q = s[static_cast<std::size_t>(b + h)];
                s[static_cast<std::size_t>(b)] = p + q;
                s[static_cast<std::size_t>(b + h)] = p - q;
            }
    outL = (y[0] + y[2] + y[4] + y[6]) * 0.5f;
    outR = (y[1] + y[3] + y[5] + y[7]) * 0.5f;
    float shim = 0;
    if (mode_ == Mode::Shimmer) shim = shimmer(0.5f * (outL + outR)) * val(p_, P::ShimAmount) * 0.55f;
    const float inj = freeze_ ? 0.0f : 0.45f;
    constexpr float norm = 0.35355339f;
    for (int k = 0; k < kLines; ++k) {
        const auto sk = static_cast<std::size_t>(k);
        lines_[sk].write(s[sk] * norm + ((k & 1) ? inR : inL) * inj + shim);
    }
}

void Reverb::processPlate(float in, float& outL, float& outR) noexcept {
    const float kIn = sr_ / kPlateRate, k = plateScale_;
    const float diff = val(p_, P::Diffusion);
    float x = plateBw_.lp(freeze_ ? 0.0f : in, OnePole::coef(std::min(18000.0f, val(p_, P::Damping) * 2.0f), sr_));
    x = plateIn_[0].process(x, 142 * kIn, 0.75f * diff);
    x = plateIn_[1].process(x, 107 * kIn, 0.75f * diff);
    x = plateIn_[2].process(x, 379 * kIn, 0.625f * diff);
    x = plateIn_[3].process(x, 277 * kIn, 0.625f * diff);

    platePhase_ += kTwoPi * val(p_, P::ModRate) / sr_;
    if (platePhase_ > kTwoPi) platePhase_ -= kTwoPi;
    const float exc = 16.0f * kIn * val(p_, P::ModDepth) * 2.0f;
    const float decay = plateDecay_;

    // Tank half A.
    float a = x + plateFbB_ * decay;
    a = apA1_.process(a, 672 * k + exc * (1.0f + std::sin(platePhase_)), -0.7f);
    dA1_.write(a);
    a = dA1_.read(4453 * k);
    if (!freeze_) a = plateDampA_.lp(a, dampA_);
    a = apA2_.process(a * decay, 1800 * k, 0.5f);
    dA2_.write(a);
    // Tank half B.
    float b = x + plateFbA_ * decay;
    b = apB1_.process(b, 908 * k + exc * (1.0f + std::cos(platePhase_)), -0.7f);
    dB1_.write(b);
    b = dB1_.read(4217 * k);
    if (!freeze_) b = plateDampB_.lp(b, dampA_);
    b = apB2_.process(b * decay, 2656 * k, 0.5f);
    dB2_.write(b);
    plateFbA_ = dA2_.read(3720 * k);
    plateFbB_ = dB2_.read(3163 * k);

    const auto t = [k](float v) { return std::max(1.0f, v * k); };
    outL = 0.6f * (dB1_.read(t(266)) + dB1_.read(t(2974)) - apB2_.line.read(t(1913)) + dB2_.read(t(1996)) - dA1_.read(t(1990)) -
                   apA2_.line.read(t(187)) - dA2_.read(t(1066)));
    outR = 0.6f * (dA1_.read(t(353)) + dA1_.read(t(3627)) - apA2_.line.read(t(1228)) + dA2_.read(t(2673)) - dB1_.read(t(2111)) -
                   apB2_.line.read(t(335)) - dB2_.read(t(121)));
}

void Reverb::processSpring(float inL, float inR, float& outL, float& outR) noexcept {
    const float in[2] = {inL, inR};
    float out[2];
    const float lpA = OnePole::coef(std::min(4500.0f, val(p_, P::Damping)), sr_);
    for (int c = 0; c < 2; ++c) {
        const auto sc = static_cast<std::size_t>(c);
        float x = (freeze_ ? 0.0f : in[c]) + springFb_[sc] * springGain_;
        // Dispersion: a chain of short all-passes smears high frequencies into the "boing".
        for (auto& ap : springAp_[sc]) x = ap.process(x, 2.0f + 0.37f * static_cast<float>(c), 0.62f);
        springLine_[sc].write(x);
        float y = springLine_[sc].read(springLen_[sc]);
        if (!freeze_) y = springLp_[sc].lp(y, lpA);
        springFb_[sc] = y;
        out[c] = y;
    }
    outL = out[0];
    outR = out[1];
}

void Reverb::process(float* l, float* r, int n) noexcept {
    const float early = val(p_, P::Early);
    const float width = val(p_, P::Width);
    const float duck = val(p_, P::Duck);
    const bool sync = val(p_, P::Sync) > 0.5f;
    const float preMs = sync ? static_cast<float>(divisionBeats(static_cast<int>(std::lround(val(p_, P::Div)))) * 60000.0 / bpm_)
                             : val(p_, P::PreDelay);
    const float preTarget = std::clamp(preMs, 0.0f, 2000.0f) * 0.001f * sr_;
    const float erScale = (mode_ == Mode::Room ? 0.5f : (mode_ == Mode::Chamber ? 0.7f : 1.0f)) * (0.4f + size_.v * 1.2f) * sr_ * 0.001f;
    const float bandLowA = OnePole::coef(250.0f, sr_), bandHighA = OnePole::coef(4000.0f, sr_);
    const float fastRel = std::exp(-1.0f / (0.03f * sr_)), slowRel = std::exp(-1.0f / (0.3f * sr_));
    double eLow = 0, eMid = 0, eHigh = 0, eWet = 0;
    float inPeak = 0, outPeak = 0;

    for (int i = 0; i < n; ++i) {
        const float dryL = l[i], dryR = r[i];
        const float mono = 0.5f * (dryL + dryR);

        // Analysis for the visuals.
        const float lo = bandLow_.lp(mono, bandLowA);
        const float hiLp = bandHigh_.lp(mono, bandHighA);
        const float hi = mono - hiLp, mid = hiLp - lo;
        eLow += lo * lo;
        eMid += mid * mid;
        eHigh += hi * hi;
        const float am = std::abs(mono);
        inPeak = std::max(inPeak, am);
        fastEnv_ = am > fastEnv_ ? am : fastEnv_ * fastRel;
        slowEnv_ = am > slowEnv_ ? slowEnv_ + (am - slowEnv_) * 0.002f : slowEnv_ * slowRel;
        if (onsetHold_ > 0) --onsetHold_;
        else if (fastEnv_ > 0.02f && fastEnv_ > slowEnv_ * 1.8f) {
            ++meters_.onsets;
            onsetHold_ = static_cast<int>(0.08f * sr_);
        }

        // Pre-delay.
        preL_.write(dryL);
        preR_.write(dryR);
        const float pre = std::max(1.0f, preTime_.next(preTarget));
        const float pl = preL_.read(pre), pr = preR_.read(pre);

        // Early reflections from the pre-delayed mono signal.
        erLine_.write(0.5f * (pl + pr));
        float erL = 0, erR = 0, g = 1.0f;
        for (int t = 0; t < 12; ++t) {
            erL += erLine_.read(kErL[t] * erScale + 1.0f) * g * ((t & 1) ? -1.0f : 1.0f);
            erR += erLine_.read(kErR[t] * erScale + 1.0f) * g * ((t & 2) ? -1.0f : 1.0f);
            g *= 0.86f;
        }
        erL *= 0.3f;
        erR *= 0.3f;
        if (freeze_) erL = erR = 0;

        // Late reverb.
        float lateL, lateR;
        switch (mode_) {
            case Mode::Plate: processPlate(0.5f * (pl + pr), lateL, lateR); break;
            case Mode::Spring: processSpring(pl, pr, lateL, lateR); break;
            default: processFdn(pl, pr, lateL, lateR); break;
        }

        float wetL = erL * early + lateL * (1.0f - early * 0.7f);
        float wetR = erR * early + lateR * (1.0f - early * 0.7f);
        wetL = highCutL_.tick(lowCutL_.tick(wetL));
        wetR = highCutR_.tick(lowCutR_.tick(wetR));
        const float m = 0.5f * (wetL + wetR), s = 0.5f * (wetL - wetR) * width;
        wetL = m + s;
        wetR = m - s;

        duckEnv_ = am > duckEnv_ ? am : duckEnv_ * duckRel_;
        const float duckGain = 1.0f - duck * std::min(1.0f, duckEnv_ * 3.0f);
        wetL *= duckGain;
        wetR *= duckGain;
        eWet += 0.5 * (wetL * wetL + wetR * wetR);

        const float mix = mix_.next(val(p_, P::Mix));
        const float outGain = out_.next(dbToGain(val(p_, P::Output)));
        const float dg = std::cos(mix * kPi * 0.5f), wg = std::sin(mix * kPi * 0.5f);
        l[i] = (dryL * dg + wetL * wg) * outGain;
        r[i] = (dryR * dg + wetR * wg) * outGain;
        outPeak = std::max(outPeak, std::max(std::abs(l[i]), std::abs(r[i])));
    }

    // Never let a numerical problem reach the speakers.
    if (!std::isfinite(outPeak)) {
        for (int i = 0; i < n; ++i) l[i] = r[i] = 0.0f;
        reset();
        outPeak = 0;
    }
    const double inv = n > 0 ? 1.0 / n : 0.0;
    meters_.low = static_cast<float>(std::sqrt(eLow * inv));
    meters_.mid = static_cast<float>(std::sqrt(eMid * inv));
    meters_.high = static_cast<float>(std::sqrt(eHigh * inv));
    meters_.wet = static_cast<float>(std::sqrt(eWet * inv));
    meters_.inPeak = inPeak;
    meters_.outPeak = outPeak;
}

}  // namespace pv
