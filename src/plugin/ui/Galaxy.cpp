#include "plugin/ui/Galaxy.h"

using namespace pv;

namespace ui {
namespace {

constexpr int kMaxParticles = 1400;
const juce::Colour kBandColour[3] = {juce::Colour(0xffff2bd6), juce::Colour(0xff00e5ff), juce::Colour(0xfffff1a8)};

juce::Image makeSprite(juce::Colour c) {
    constexpr int s = 64;
    juce::Image img(juce::Image::ARGB, s, s, true);
    juce::Graphics g(img);
    const juce::Point<float> ctr(s * 0.5f, s * 0.5f);
    juce::ColourGradient glow(c.withAlpha(0.55f), ctr, c.withAlpha(0.0f), {s * 0.5f, 0.0f}, true);
    glow.addColour(0.25, c.withAlpha(0.3f));
    g.setGradientFill(glow);
    g.fillEllipse(0, 0, s, s);
    juce::ColourGradient core(juce::Colours::white, ctr, c.withAlpha(0.0f), {s * 0.5f, s * 0.36f}, true);
    g.setGradientFill(core);
    g.fillEllipse(s * 0.36f, s * 0.36f, s * 0.28f, s * 0.28f);
    return img;
}

}  // namespace

Galaxy::Galaxy(PatVerbProcessor& p) : proc_(p) {
    particles_.reserve(kMaxParticles);
    for (int b = 0; b < 3; ++b) sprites_[static_cast<std::size_t>(b)] = makeSprite(kBandColour[b]);
    setTooltip("Drag: left/right changes SIZE, up/down changes DECAY");
    setMouseCursor(juce::MouseCursor::UpDownLeftRightResizeCursor);
    startTimerHz(60);
}

void Galaxy::resized() {
    // Static starfield, rebuilt only when the size changes.
    stars_ = juce::Image(juce::Image::ARGB, juce::jmax(1, getWidth()), juce::jmax(1, getHeight()), true);
    juce::Graphics g(stars_);
    juce::Random r(42);
    for (int i = 0; i < getWidth() * getHeight() / 900; ++i) {
        const float x = r.nextFloat() * getWidth(), y = r.nextFloat() * getHeight();
        const float s = r.nextFloat() < 0.08f ? 1.8f : 1.0f;
        g.setColour(juce::Colours::white.withAlpha(0.08f + 0.35f * r.nextFloat() * r.nextFloat()));
        g.fillEllipse(x, y, s, s);
    }
}

void Galaxy::spawn(int band, int count, float energy) {
    const float size = proc_.value(P::Size);
    for (int i = 0; i < count && static_cast<int>(particles_.size()) < kMaxParticles; ++i) {
        Particle p;
        p.band = band;
        p.r = rng_.nextFloat() * 0.06f;
        // Two spiral arms; lows start a little wider, highs closer to the core.
        p.angle = rng_.nextFloat() * juce::MathConstants<float>::twoPi + (rng_.nextBool() ? 0.0f : juce::MathConstants<float>::pi);
        p.vr = 1.5f * (0.25f + 0.9f * rng_.nextFloat()) * (0.35f + size) * (0.6f + juce::jmin(1.0f, energy * 4.0f)) * (band == 0 ? 1.15f : (band == 2 ? 0.8f : 1.0f));
        p.life = 1.0f;
        const float visualLife = juce::jlimit(0.4f, 14.0f, proc_.value(P::Decay));
        p.decayRate = 1.0f / (visualLife * (0.6f + 0.8f * rng_.nextFloat()));
        p.size = (band == 0 ? 22.0f : (band == 1 ? 15.0f : 10.0f)) * (0.6f + 0.8f * rng_.nextFloat());
        p.twinkle = rng_.nextFloat() * 6.28f;
        particles_.push_back(p);
    }
}

void Galaxy::step(float dt, float low, float mid, float high, float wet, int newOnsets) {
    const bool frozen = proc_.value(P::Freeze) > 0.5f;
    const float size = proc_.value(P::Size);
    const float modRate = proc_.value(P::ModRate);
    const auto mode = static_cast<Mode>(juce::roundToInt(proc_.value(P::Mode)));

    if (!frozen) {
        // Continuous emission from the input energy, and a burst on each detected hit.
        const float levels[3] = {low, mid, high * 1.6f};
        for (int b = 0; b < 3; ++b) {
            const float rate = juce::jmin(1.0f, levels[b] * 3.0f) * 140.0f * dt;
            int n = static_cast<int>(rate);
            if (rng_.nextFloat() < rate - static_cast<float>(n)) ++n;
            spawn(b, n, levels[b]);
        }
        if (newOnsets > 0) {
            const float total = low + mid + high + 1e-6f;
            for (int b = 0; b < 3; ++b) spawn(b, juce::roundToInt(70.0f * newOnsets * ((b == 0 ? low : b == 1 ? mid : high) / total + 0.15f)), 0.5f);
            corePulse_ = 1.0f;
        }
    }

    spin_ += dt * (frozen ? 0.05f : 0.15f + modRate * 0.25f);
    const float drag = std::exp(-dt * 1.5f);
    const float ringR = 0.25f + size * 0.6f;
    for (auto& p : particles_) {
        p.r += p.vr * dt;
        p.vr *= frozen ? 1.0f : drag;
        if (p.r > ringR * 1.4f) p.vr -= dt * 0.4f;  // gentle pull so the galaxy keeps its shape
        // Differential rotation: inner particles orbit faster (a real galaxy look).
        const float omega = (frozen ? 0.08f : 0.35f + modRate * 0.35f) / (p.r + 0.25f);
        p.angle += omega * dt * (mode == Mode::Spring ? 1.0f + 0.4f * std::sin(p.twinkle * 3.0f + spin_ * 9.0f) : 1.0f);
        p.twinkle += dt * 4.0f;
        if (!frozen) p.life -= p.decayRate * dt;
        if (mode == Mode::Shimmer) p.r += dt * 0.02f;  // shimmer drifts outwards forever
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) { return p.life <= 0.0f || p.r > 2.5f; }),
                     particles_.end());
    corePulse_ *= std::exp(-dt * 4.0f);
    wetGlow_ += (juce::jmin(1.0f, wet * 4.0f) - wetGlow_) * juce::jmin(1.0f, dt * 6.0f);
}

void Galaxy::timerCallback() {
    auto& m = proc_.meters;
    const int onsets = m.onsets.load();
    const int fresh = juce::jlimit(0, 4, onsets - lastOnsets_);
    lastOnsets_ = onsets;
    step(1.0f / 60.0f, m.low.load(), m.mid.load(), m.high.load(), m.wet.load(), fresh);
    repaint();
}

void Galaxy::simulate(float seconds) {
    for (float t = 0; t < seconds; t += 1.0f / 60.0f) {
        const bool hit = std::fmod(t, 0.5f) < 1.0f / 60.0f;
        step(1.0f / 60.0f, 0.12f, 0.08f, 0.05f, 0.2f, hit ? 1 : 0);
    }
    repaint();
}

void Galaxy::paint(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat();
    const auto c = r.getCentre();
    const float radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
    const float width = proc_.value(P::Width);
    const auto mode = static_cast<Mode>(juce::roundToInt(proc_.value(P::Mode)));
    const bool frozen = proc_.value(P::Freeze) > 0.5f;
    const float stretchX = (1.1f + width * 0.55f) * (mode == Mode::Plate ? 1.25f : 1.0f);
    const float stretchY = mode == Mode::Plate ? 0.45f : 0.62f;

    g.fillAll(juce::Colour(0xff04050a));
    {
        juce::ColourGradient nebula(juce::Colour(0xff15103a).withAlpha(0.9f), c, juce::Colour(0xff04050a), {c.x + radius * 1.6f, c.y}, true);
        g.setGradientFill(nebula);
        g.fillRect(r);
    }
    g.drawImageAt(stars_, 0, 0);

    // Faint ring where the galaxy settles (follows SIZE).
    const float ringR = (0.25f + proc_.value(P::Size) * 0.6f) * radius;
    g.setColour((frozen ? neonCyan : neonViolet).withAlpha(0.10f));
    g.drawEllipse(juce::Rectangle<float>(ringR * 2 * stretchX, ringR * 2 * stretchY).withCentre(c), 1.0f);

    // Core: pulses on hits, glows with the wet level.
    const float coreR = radius * (0.10f + 0.06f * corePulse_ + 0.05f * wetGlow_);
    juce::ColourGradient core(juce::Colours::white.withAlpha(0.35f + 0.5f * corePulse_), c, neonViolet.withAlpha(0.0f), {c.x + coreR * 2.2f, c.y}, true);
    core.addColour(0.3, (frozen ? neonCyan : neonPink).withAlpha(0.25f + 0.3f * wetGlow_));
    g.setGradientFill(core);
    g.fillEllipse(juce::Rectangle<float>(coreR * 4.4f, coreR * 4.4f).withCentre(c));

    // Particles.
    const float glowBoost = 0.45f + 0.55f * wetGlow_;
    const float cloudScale = mode == Mode::Cloud ? 1.8f : (mode == Mode::Room ? 0.75f : 1.0f);
    for (const auto& p : particles_) {
        const float a = p.angle + spin_;
        const float x = c.x + std::cos(a) * p.r * radius * stretchX;
        const float y = c.y + std::sin(a) * p.r * radius * stretchY - (mode == Mode::Shimmer ? (1.0f - p.life) * radius * 0.25f : 0.0f);
        const float tw = 0.75f + 0.25f * std::sin(p.twinkle);
        const float alpha = juce::jlimit(0.0f, 1.0f, std::pow(p.life, 1.4f) * glowBoost * tw);
        if (alpha < 0.01f) continue;
        const float s = p.size * cloudScale * (0.6f + 0.4f * p.life) * (radius / 220.0f);
        g.setOpacity(alpha);
        g.drawImage(sprites_[static_cast<std::size_t>(p.band)], juce::Rectangle<float>(s * 2, s * 2).withCentre({x, y}));
    }
    g.setOpacity(1.0f);

    // Labels.
    g.setFont(font(40.0f, true));
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawText(juce::String(std::string(choiceName(Choices::Mode, juce::roundToInt(proc_.value(P::Mode))))).toUpperCase(),
               r.reduced(24.0f, 18.0f), juce::Justification::bottomLeft);
    g.setFont(font(12.0f, true));
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.drawText("SIZE " + formatParam(P::Size, proc_.value(P::Size)) + "   DECAY " + formatParam(P::Decay, proc_.value(P::Decay)),
               r.reduced(24.0f, 14.0f), juce::Justification::bottomRight);
    if (dragging_) {
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(font(15.0f, true));
        g.drawText("< SIZE >     ^ DECAY v", r.reduced(10.0f), juce::Justification::centredTop);
    }
}

void Galaxy::mouseDown(const juce::MouseEvent&) {
    dragging_ = true;
    dragSize_ = proc_.param(P::Size)->getValue();
    dragDecay_ = proc_.param(P::Decay)->getValue();
    proc_.param(P::Size)->beginChangeGesture();
    proc_.param(P::Decay)->beginChangeGesture();
}

void Galaxy::mouseDrag(const juce::MouseEvent& e) {
    const float dx = static_cast<float>(e.getDistanceFromDragStartX()) / static_cast<float>(juce::jmax(1, getWidth()));
    const float dy = static_cast<float>(e.getDistanceFromDragStartY()) / static_cast<float>(juce::jmax(1, getHeight()));
    proc_.param(P::Size)->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, dragSize_ + dx * 1.2f));
    proc_.param(P::Decay)->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, dragDecay_ - dy * 1.2f));
}

void Galaxy::mouseUp(const juce::MouseEvent&) {
    dragging_ = false;
    proc_.param(P::Size)->endChangeGesture();
    proc_.param(P::Decay)->endChangeGesture();
}

}  // namespace ui
