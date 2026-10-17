// The particle galaxy: every hit in the input bursts into glowing particles that swirl around
// the core and fade as slowly as the reverb tail. Lows are pink/violet, mids cyan, highs gold.
// Size opens the galaxy, decay sets how long particles live, width stretches it, mod rate
// spins it, freeze stops time. Drag on it: left/right = size, up/down = decay.
#pragma once

#include "plugin/PluginProcessor.h"
#include "plugin/ui/Theme.h"

#include <array>
#include <vector>

namespace ui {

class Galaxy final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer {
public:
    explicit Galaxy(PatVerbProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    // Advances the simulation with fake input (used by the diagnostic snapshot).
    void simulate(float seconds);

private:
    struct Particle {
        float r, angle, vr, life, decayRate, size, twinkle;
        int band;
    };
    void timerCallback() override;
    void step(float dt, float low, float mid, float high, float wet, int newOnsets);
    void spawn(int band, int count, float energy);

    PatVerbProcessor& proc_;
    std::vector<Particle> particles_;
    std::array<juce::Image, 3> sprites_;
    juce::Image stars_;
    juce::Random rng_;
    int lastOnsets_ = 0;
    float corePulse_ = 0, wetGlow_ = 0, spin_ = 0;
    float dragSize_ = 0, dragDecay_ = 0;
    bool dragging_ = false;
};

}  // namespace ui
