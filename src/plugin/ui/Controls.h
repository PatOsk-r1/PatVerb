// Neon controls bound to parameters.
#pragma once

#include "plugin/PluginProcessor.h"
#include "plugin/ui/Theme.h"

namespace ui {

class NeonKnob final : public juce::Slider {
public:
    NeonKnob(PatVerbProcessor& p, pv::P param, const juce::String& label, juce::Colour colour);
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void setColour(juce::Colour c) {
        colour_ = c;
        repaint();
    }

private:
    pv::P param_;
    juce::String label_;
    juce::Colour colour_;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment_;
};

class ParamToggle final : public juce::ToggleButton {
public:
    ParamToggle(PatVerbProcessor& p, pv::P param, const juce::String& text);

private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment_;
};

class ParamCombo final : public juce::ComboBox {
public:
    ParamCombo(PatVerbProcessor& p, pv::P param);

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment_;
};

// Big glowing FREEZE switch.
class FreezeButton final : public juce::ToggleButton {
public:
    explicit FreezeButton(PatVerbProcessor& p);
    void paintButton(juce::Graphics&, bool over, bool down) override;

private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment_;
};

// The seven algorithms as pill buttons.
class ModeBar final : public juce::Component, public juce::SettableTooltipClient {
public:
    explicit ModeBar(PatVerbProcessor& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override {
        hover_ = -1;
        repaint();
    }
    void tick();

private:
    juce::Rectangle<float> pill(int i) const;
    PatVerbProcessor& proc_;
    int mode_ = -1, hover_ = -1;
};

}  // namespace ui
