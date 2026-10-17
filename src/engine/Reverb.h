// PatVerb engine: pre-delay, early reflections, seven late-reverb algorithms, tail filters,
// ducking, freeze, width and the dry/wet mix. Pure C++, no JUCE.
#pragma once

#include "engine/Dsp.h"
#include "engine/Params.h"

#include <array>

namespace pv {

// Analysis of the current block for the visuals (filled by process()).
struct Meters {
    float low = 0, mid = 0, high = 0;  // input RMS per band
    float wet = 0;                     // wet signal RMS
    float inPeak = 0, outPeak = 0;
    int onsets = 0;                    // running count of detected hits
};

class Reverb {
public:
    void prepare(double sampleRate, int maxBlock);
    void reset() noexcept;
    // Plain parameter values (real units, indexed by P), once per block.
    void setParams(const float* plain, double bpm) noexcept;
    // In place: stereo in, mixed stereo out.
    void process(float* l, float* r, int n) noexcept;
    const Meters& meters() const noexcept { return meters_; }

private:
    void processFdn(float inL, float inR, float& outL, float& outR) noexcept;
    void processPlate(float in, float& outL, float& outR) noexcept;
    void processSpring(float inL, float inR, float& outL, float& outR) noexcept;
    float shimmer(float x) noexcept;
    void updateLengths() noexcept;

    float sr_ = 48000;
    std::array<float, kNumParams> p_{};
    Mode mode_ = Mode::Hall;
    bool freeze_ = false;
    double bpm_ = 120;

    // Shared
    DelayLine preL_, preR_, erLine_;
    Smooth mix_, out_, preTime_, size_;
    Biquad lowCutL_, lowCutR_, highCutL_, highCutR_;
    float duckEnv_ = 0, duckRel_ = 0.999f;
    float lastLowCut_ = -1, lastHighCut_ = -1;

    // FDN (Hall, Room, Chamber, Shimmer, Cloud)
    static constexpr int kLines = 8;
    std::array<DelayLine, kLines> lines_;
    std::array<float, kLines> len_{}, gain_{}, phase_{};
    std::array<OnePole, kLines> damp_, bass_;
    std::array<Allpass, 8> diffL_, diffR_;
    float dampA_ = 0, bassA_ = 0, modDepth_ = 0;
    int numDiffusers_ = 4;
    std::array<float, 8> diffLen_{};
    float diffG_ = 0.6f;

    // Shimmer pitch shifter
    DelayLine shimLine_;
    float shimPhase_ = 0, shimRatio_ = 2;

    // Plate (Dattorro)
    std::array<Allpass, 4> plateIn_;
    Allpass apA1_, apA2_, apB1_, apB2_;
    DelayLine dA1_, dA2_, dB1_, dB2_;
    OnePole plateDampA_, plateDampB_, plateBw_;
    float plateFbA_ = 0, plateFbB_ = 0, plateScale_ = 1, plateDecay_ = 0.5f, platePhase_ = 0;

    // Spring
    std::array<std::array<Allpass, 12>, 2> springAp_;
    std::array<DelayLine, 2> springLine_;
    std::array<OnePole, 2> springLp_;
    std::array<float, 2> springFb_{}, springLen_{};
    float springGain_ = 0.5f;

    // Analysis
    Meters meters_;
    OnePole bandLow_, bandHigh_;
    float fastEnv_ = 0, slowEnv_ = 0;
    int onsetHold_ = 0;
};

}  // namespace pv
