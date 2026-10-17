// Real-time building blocks for the reverb (no allocation outside allocate()/prepare()).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

namespace pv {

inline constexpr float kPi = std::numbers::pi_v<float>;
inline constexpr float kTwoPi = 2.0f * kPi;

inline float dbToGain(float db) noexcept { return std::pow(10.0f, db * 0.05f); }
inline float gainToDb(float g) noexcept { return 20.0f * std::log10(std::max(g, 1e-9f)); }

class DelayLine {
public:
    void allocate(int maxSamples) {
        int size = 1;
        while (size < maxSamples + 4) size <<= 1;
        buf_.assign(static_cast<std::size_t>(size), 0.0f);
        mask_ = size - 1;
        pos_ = 0;
    }
    void clear() noexcept { std::fill(buf_.begin(), buf_.end(), 0.0f); }
    void write(float x) noexcept {
        buf_[static_cast<std::size_t>(pos_)] = x;
        pos_ = (pos_ + 1) & mask_;
    }
    // d samples behind the most recent write (d >= 1); linear interpolation.
    float read(float d) const noexcept {
        const float rp = static_cast<float>(pos_) - d;
        const float fl = std::floor(rp);
        const int i0 = static_cast<int>(fl) & mask_;
        const int i1 = (i0 + 1) & mask_;
        const float f = rp - fl;
        return buf_[static_cast<std::size_t>(i0)] + (buf_[static_cast<std::size_t>(i1)] - buf_[static_cast<std::size_t>(i0)]) * f;
    }
    float readInt(int d) const noexcept { return buf_[static_cast<std::size_t>((pos_ - d) & mask_)]; }
    int capacity() const noexcept { return mask_ - 2; }

private:
    std::vector<float> buf_{0.0f};
    int pos_ = 0, mask_ = 0;
};

// Schroeder all-pass around a delay line; the line stays readable for output taps.
struct Allpass {
    DelayLine line;
    float process(float x, float delay, float g) noexcept {
        const float d = line.read(delay);
        const float w = x + g * d;
        line.write(w);
        return d - g * w;
    }
};

struct OnePole {  // low-pass; high-pass = input - low-pass
    float z = 0;
    float lp(float x, float a) noexcept { return z = x + (z - x) * a; }
    static float coef(float hz, float sr) noexcept { return std::exp(-kTwoPi * std::min(hz, sr * 0.45f) / sr); }
};

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void lowpass(float sr, float f, float q = 0.7071f) noexcept { set(sr, f, q, false); }
    void highpass(float sr, float f, float q = 0.7071f) noexcept { set(sr, f, q, true); }
    float tick(float x) noexcept {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() noexcept { z1 = z2 = 0; }

private:
    void set(float sr, float f, float q, bool high) noexcept {
        const float w = kTwoPi * std::clamp(f, 10.0f, sr * 0.45f) / sr, c = std::cos(w), al = std::sin(w) / (2.0f * q);
        const float a0 = 1 + al;
        if (high) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; }
        else { b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; }
        b0 /= a0; b1 /= a0; b2 /= a0;
        a1 = -2 * c / a0;
        a2 = (1 - al) / a0;
    }
};

struct Smooth {
    float v = 0, coef = 0.0015f;
    bool init = false;
    float next(float t) noexcept {
        if (!init) { v = t; init = true; }
        return v += (t - v) * coef;
    }
};

}  // namespace pv
