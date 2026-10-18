// PatVerb look: deep-space black, neon accents.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

inline const juce::Colour bg{0xff07080d};
inline const juce::Colour panel{0xff10131b};
inline const juce::Colour raised{0xff1c212c};
inline const juce::Colour hover{0xff252b38};
inline const juce::Colour line{0xff2a3040};
inline const juce::Colour text{0xffe6e9f0};
inline const juce::Colour dim{0xff8a92a6};
inline const juce::Colour faint{0xff4f5668};
inline const juce::Colour accent{0xff3fd2ff};
inline const juce::Colour accentDeep{0xff1b6f8c};
inline const juce::Colour screen{0xff090b10};
inline const juce::Colour neonCyan{0xff00f0ff};
inline const juce::Colour neonPink{0xffff2bd6};
inline const juce::Colour neonLime{0xff39ff14};
inline const juce::Colour neonOrange{0xffff9a1f};
inline const juce::Colour neonViolet{0xffa565ff};
inline const juce::Colour neonGold{0xfffff3a0};
inline const juce::Colour warn{0xffff5a6e};

juce::Font font(float height, bool bold = false);

// Rounded panel with an optional title strip.
void drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title = {});
// Dark "screen" area used by the wavetable, envelope, LFO and filter displays.
void drawScreen(juce::Graphics& g, juce::Rectangle<float> r);

class LookAndFeel final : public juce::LookAndFeel_V4 {
public:
    LookAndFeel();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int h) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(12.5f); }
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override { return font(13.5f); }
    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    // Menus open inside the plugin window instead of as separate desktop windows.
    juce::Component* getParentComponentForMenuOptions(const juce::PopupMenu::Options& o) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle,
                          juce::Slider&) override;
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                       int thumbSize, bool over, bool down) override;
};

}  // namespace ui
