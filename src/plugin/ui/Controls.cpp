#include "plugin/ui/Controls.h"

using namespace pv;

namespace ui {
namespace {
constexpr float kStart = juce::MathConstants<float>::pi * 1.25f;
constexpr float kEnd = juce::MathConstants<float>::pi * 2.75f;
juce::String id(P p) { return juce::String(std::string(info(p).key)); }
}  // namespace

NeonKnob::NeonKnob(PatVerbProcessor& p, P param, const juce::String& label, juce::Colour colour)
    : param_(param), label_(label), colour_(colour), attachment_(p.apvts, id(param), *this) {
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(kStart, kEnd, true);
    setMouseDragSensitivity(220);
    setVelocityModeParameters(0.6, 1, 0.08, true, juce::ModifierKeys::shiftModifier);
    if (auto* prm = p.param(param)) setDoubleClickReturnValue(true, prm->convertFrom0to1(prm->getDefaultValue()));
    setTooltip(juce::String(std::string(info(param).name)) + " (double-click: reset, Shift: fine)");
}

void NeonKnob::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    const auto labelArea = b.removeFromBottom(15.0f);
    const float size = juce::jmin(b.getWidth(), b.getHeight(), 72.0f);
    const auto c = b.getCentre();
    const float r = size * 0.5f - 6.0f;
    if (r < 4.0f) return;
    const float norm = static_cast<float>(valueToProportionOfLength(getValue()));
    const float ang = kStart + (kEnd - kStart) * norm;
    const auto col = isEnabled() ? colour_ : faint;

    juce::Path track;
    track.addCentredArc(c.x, c.y, r, r, 0.0f, kStart, kEnd, true);
    g.setColour(hover);
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (norm > 0.002f) {
        juce::Path val;
        val.addCentredArc(c.x, c.y, r, r, 0.0f, kStart, ang, true);
        g.setColour(col.withAlpha(0.25f));  // glow
        g.strokePath(val, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(col);
        g.strokePath(val, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float body = r * 0.7f;
    g.setGradientFill(juce::ColourGradient(raised.brighter(0.2f), c.x, c.y - body, juce::Colour(0xff0a0c12), c.x, c.y + body, false));
    g.fillEllipse(juce::Rectangle<float>(body * 2, body * 2).withCentre(c));
    g.setColour(col.withAlpha(0.35f));
    g.drawEllipse(juce::Rectangle<float>(body * 2, body * 2).withCentre(c), 1.0f);
    const juce::Point<float> dir(std::sin(ang), -std::cos(ang));
    g.setColour(juce::Colours::white);
    g.drawLine({c + dir * (body * 0.3f), c + dir * (body * 0.9f)}, 2.0f);

    const bool showValue = isMouseOverOrDragging();
    g.setColour(showValue ? text : dim);
    g.setFont(font(11.0f, !showValue));
    g.drawText(showValue ? formatParam(param_, static_cast<float>(getValue())) : label_, labelArea.expanded(8.0f, 0.0f), juce::Justification::centred, false);
}

ParamToggle::ParamToggle(PatVerbProcessor& p, P param, const juce::String& text)
    : juce::ToggleButton(text), attachment_(p.apvts, id(param), *this) {}

ParamCombo::ParamCombo(PatVerbProcessor& p, P param) {
    const auto& pi = info(param);
    for (int i = 0; i < numChoices(pi.choices); ++i) addItem(juce::String(std::string(choiceName(pi.choices, i))), i + 1);
    attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, id(param), *this);
}

FreezeButton::FreezeButton(PatVerbProcessor& p) : juce::ToggleButton("FREEZE"), attachment_(p.apvts, id(P::Freeze), *this) {
    setTooltip("Freeze: holds the reverb tail forever and ignores new input");
}

void FreezeButton::paintButton(juce::Graphics& g, bool over, bool) {
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    const bool on = getToggleState();
    if (on) {
        g.setColour(neonCyan.withAlpha(0.18f));
        g.fillRoundedRectangle(r.expanded(1.0f), r.getHeight() * 0.5f);
    }
    g.setColour(on ? neonCyan.withAlpha(0.25f) : (over ? hover : raised.withAlpha(0.8f)));
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
    g.setColour(on ? neonCyan : line);
    g.drawRoundedRectangle(r, r.getHeight() * 0.5f, on ? 2.0f : 1.0f);
    g.setColour(on ? juce::Colours::white : dim);
    g.setFont(font(14.0f, true));
    g.drawText(on ? "FROZEN" : "FREEZE", r, juce::Justification::centred);
}

ModeBar::ModeBar(PatVerbProcessor& p) : proc_(p) { setTooltip("Reverb algorithm"); }

juce::Rectangle<float> ModeBar::pill(int i) const {
    const float w = static_cast<float>(getWidth()) / 7.0f;
    return {w * static_cast<float>(i) + 2.0f, 2.0f, w - 4.0f, static_cast<float>(getHeight()) - 4.0f};
}

void ModeBar::tick() {
    const int m = juce::roundToInt(proc_.value(P::Mode));
    if (m != mode_) {
        mode_ = m;
        repaint();
    }
}

void ModeBar::paint(juce::Graphics& g) {
    for (int i = 0; i < 7; ++i) {
        const auto r = pill(i);
        const bool sel = i == mode_;
        g.setColour(sel ? neonPink.withAlpha(0.22f) : (i == hover_ ? hover : juce::Colour(0xb0101420)));
        g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
        g.setColour(sel ? neonPink : line);
        g.drawRoundedRectangle(r, r.getHeight() * 0.5f, sel ? 1.6f : 1.0f);
        g.setColour(sel ? juce::Colours::white : dim);
        g.setFont(font(12.0f, true));
        g.drawText(juce::String(std::string(choiceName(Choices::Mode, i))).toUpperCase(), r, juce::Justification::centred);
    }
}

void ModeBar::mouseDown(const juce::MouseEvent& e) {
    for (int i = 0; i < 7; ++i)
        if (pill(i).contains(e.position)) {
            if (auto* p = proc_.param(P::Mode)) {
                p->beginChangeGesture();
                proc_.setValue(P::Mode, static_cast<float>(i));
                p->endChangeGesture();
            }
            tick();
        }
}

void ModeBar::mouseMove(const juce::MouseEvent& e) {
    int h = -1;
    for (int i = 0; i < 7; ++i)
        if (pill(i).contains(e.position)) h = i;
    if (h != hover_) {
        hover_ = h;
        repaint();
    }
}

}  // namespace ui
