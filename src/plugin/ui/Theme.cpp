#include "plugin/ui/Theme.h"


namespace ui {

namespace {
const juce::String kFace{"Segoe UI"};
}

juce::Font font(float height, bool bold) {
    return bold ? juce::Font(juce::FontOptions(kFace, "Semibold", height)) : juce::Font(juce::FontOptions(kFace, height, juce::Font::plain));
}

void drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) {
    const auto rf = r.toFloat();
    g.setColour(panel);
    g.fillRoundedRectangle(rf, 8.0f);
    g.setColour(line);
    g.drawRoundedRectangle(rf.reduced(0.5f), 8.0f, 1.0f);
    if (title.isNotEmpty()) {
        g.setColour(dim);
        g.setFont(font(11.5f, true));
        g.drawText(title.toUpperCase(), r.reduced(10, 0).withHeight(24), juce::Justification::centredLeft);
    }
}

void drawScreen(juce::Graphics& g, juce::Rectangle<float> r) {
    g.setColour(screen);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(line.withAlpha(0.8f));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
}

LookAndFeel::LookAndFeel() {
    setDefaultSansSerifTypefaceName(kFace);
    using CS = juce::LookAndFeel_V4::ColourScheme;
    setColourScheme(CS{bg, panel, raised, line, text, accent, bg, raised, text});
    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::TextButton::buttonColourId, raised);
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::ComboBox::backgroundColourId, raised);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, line);
    setColour(juce::ComboBox::arrowColourId, dim);
    setColour(juce::PopupMenu::backgroundColourId, raised);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::headerTextColourId, dim);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, hover);
    setColour(juce::PopupMenu::highlightedTextColourId, accent);
    setColour(juce::Label::textColourId, text);
    setColour(juce::TextEditor::backgroundColourId, raised);
    setColour(juce::TextEditor::textColourId, text);
    setColour(juce::TextEditor::outlineColourId, line);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.35f));
    setColour(juce::CaretComponent::caretColourId, accent);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TooltipWindow::backgroundColourId, raised);
    setColour(juce::TooltipWindow::textColourId, text);
    setColour(juce::TooltipWindow::outlineColourId, line);
    setColour(juce::AlertWindow::backgroundColourId, panel);
    setColour(juce::AlertWindow::textColourId, text);
    setColour(juce::AlertWindow::outlineColourId, line);
    setColour(juce::ScrollBar::thumbColourId, hover);
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = b.getToggleState();
    if (b.getProperties()["swatch"]) {  // colour picker swatch
        const auto c = b.findColour(juce::TextButton::buttonColourId);
        g.setColour(c.withAlpha(0.25f));
        g.fillEllipse(r);
        g.setColour(c);
        g.fillEllipse(r.reduced(4.0f));
        if (on || over) {
            g.setColour(on ? juce::Colours::white : c.brighter());
            g.drawEllipse(r.reduced(1.0f), on ? 2.0f : 1.0f);
        }
        return;
    }
    const bool tab = b.getProperties()["tab"];
    juce::Colour fill = on ? (tab ? hover : accentDeep) : raised;
    if (tab && !on) fill = juce::Colours::transparentBlack;
    if (down) fill = fill.brighter(0.1f);
    else if (over) fill = on ? fill.brighter(0.05f) : hover;
    g.setColour(fill);
    g.fillRoundedRectangle(r, 5.0f);
    if (!tab) {
        g.setColour(on ? accent.withAlpha(0.6f) : line);
        g.drawRoundedRectangle(r, 5.0f, 1.0f);
    } else if (on) {
        g.setColour(accent);
        g.fillRoundedRectangle(r.withTop(r.getBottom() - 2.0f).reduced(6.0f, 0.0f), 1.0f);
    }
}

void LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    g.setFont(getTextButtonFont(b, b.getHeight()));
    const bool tab = b.getProperties()["tab"];
    g.setColour(!b.isEnabled() ? faint : (b.getToggleState() ? (tab ? accent : text) : (tab ? dim : text)));
    g.drawText(b.getButtonText(), b.getLocalBounds().reduced(4, 0), juce::Justification::centred, true);
}

juce::Font LookAndFeel::getTextButtonFont(juce::TextButton& b, int h) {
    return font(juce::jmin(13.0f, static_cast<float>(h) * 0.55f), b.getProperties()["tab"]);
}

// Toggles are drawn as small power LEDs with an optional label to the right.
void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool) {
    const auto r = b.getLocalBounds().toFloat();
    const float d = juce::jmin(14.0f, r.getHeight() - 2.0f);
    const auto led = juce::Rectangle<float>(d, d).withCentre({r.getX() + d * 0.5f + 1.0f, r.getCentreY()});
    const bool on = b.getToggleState();
    g.setColour(on ? accent : (over ? hover.brighter(0.2f) : hover));
    g.fillEllipse(led);
    if (on) {
        g.setColour(accent.withAlpha(0.25f));
        g.fillEllipse(led.expanded(3.0f));
    }
    g.setColour(on ? bg : dim);
    juce::Path power;
    const auto c = led.getCentre();
    const float rr = d * 0.27f;
    power.addCentredArc(c.x, c.y, rr, rr, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
    g.strokePath(power, juce::PathStrokeType(1.4f));
    g.drawLine(c.x, c.y - rr - 1.0f, c.x, c.y - 0.5f, 1.4f);
    if (b.getButtonText().isNotEmpty()) {
        g.setColour(on ? text : dim);
        g.setFont(font(11.5f, true));
        g.drawText(b.getButtonText(), r.withTrimmedLeft(d + 6.0f), juce::Justification::centredLeft, true);
    }
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    const auto r = juce::Rectangle<float>(0, 0, static_cast<float>(w), static_cast<float>(h)).reduced(0.5f);
    g.setColour(box.isMouseOver(true) ? hover : raised);
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(line);
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
    juce::Path arrow;
    const float ax = static_cast<float>(w) - 11.0f, ay = static_cast<float>(h) * 0.5f;
    arrow.addTriangle(ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 2.5f);
    g.setColour(dim);
    g.fillPath(arrow);
}

void LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    label.setBounds(4, 0, box.getWidth() - 18, box.getHeight());
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centredLeft);
}

juce::Component* LookAndFeel::getParentComponentForMenuOptions(const juce::PopupMenu::Options& o) {
    if (auto* parent = o.getParentComponent()) return parent;
    if (auto* target = o.getTargetComponent()) return target->getTopLevelComponent();
    return nullptr;
}

void LookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(raised);
    g.setColour(line);
    g.drawRect(0, 0, w, h);
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                   juce::Slider::SliderStyle, juce::Slider& s) {
    // Horizontal bipolar bar (used for modulation amounts).
    const auto r = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y) + static_cast<float>(h) * 0.5f - 3.0f,
                                          static_cast<float>(w), 6.0f);
    g.setColour(hover);
    g.fillRoundedRectangle(r, 3.0f);
    const float zero = s.getMinimum() < 0 ? static_cast<float>(s.valueToProportionOfLength(0.0)) * static_cast<float>(w) + static_cast<float>(x)
                                           : static_cast<float>(x);
    const auto bar = juce::Rectangle<float>::leftTopRightBottom(juce::jmin(zero, pos), r.getY(), juce::jmax(zero, pos), r.getBottom());
    const auto c = s.findColour(juce::Slider::trackColourId);
    g.setColour(c);
    g.fillRoundedRectangle(bar, 3.0f);
    g.setColour(text);
    g.fillEllipse(juce::Rectangle<float>(8, 8).withCentre({pos, r.getCentreY()}));
}

void LookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                                int thumbSize, bool over, bool) {
    auto thumb = vertical ? juce::Rectangle<int>(x, thumbStart, w, thumbSize) : juce::Rectangle<int>(thumbStart, y, thumbSize, h);
    g.setColour(over ? hover.brighter(0.15f) : hover);
    g.fillRoundedRectangle(thumb.toFloat().reduced(2.0f), 3.0f);
}

}  // namespace ui
