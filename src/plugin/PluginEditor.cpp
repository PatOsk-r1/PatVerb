#include "plugin/PluginEditor.h"

#include "plugin/Presets.h"
#include "plugin/ui/KeyboardFocus.h"

#include <map>

using namespace pv;
using namespace ui;

namespace {
constexpr int kTopH = 48;
constexpr int kGalaxyBottom = 412;

struct Group {
    juce::String title;
    juce::Colour colour;
    juce::Rectangle<int> bounds;
};
}  // namespace

class PatVerbEditor::Content final : public juce::Component {
public:
    explicit Content(PatVerbProcessor& p)
        : proc(p),
          galaxy(p),
          modes(p),
          freeze(p),
          size(p, P::Size, "SIZE", neonCyan),
          decay(p, P::Decay, "DECAY", neonCyan),
          predelay(p, P::PreDelay, "PRE-DELAY", neonCyan),
          diffusion(p, P::Diffusion, "DIFFUSION", neonCyan),
          damping(p, P::Damping, "DAMPING", neonPink),
          bassCut(p, P::BassCut, "BASS CUT", neonPink),
          lowCut(p, P::LowCut, "LOW CUT", neonPink),
          highCut(p, P::HighCut, "HIGH CUT", neonPink),
          early(p, P::Early, "EARLY/LATE", neonGold),
          mix(p, P::Mix, "MIX", neonGold),
          output(p, P::Output, "OUTPUT", neonGold),
          modRate(p, P::ModRate, "RATE", neonLime),
          modDepth(p, P::ModDepth, "DEPTH", neonLime),
          width(p, P::Width, "WIDTH", neonLime),
          duck(p, P::Duck, "DUCKING", neonOrange),
          duckRel(p, P::DuckRelease, "RELEASE", neonOrange),
          shimAmount(p, P::ShimAmount, "AMOUNT", neonViolet),
          sync(p, P::Sync, "BPM"),
          div(p, P::Div),
          shimPitch(p, P::ShimPitch) {
        for (juce::Component* c : std::initializer_list<juce::Component*>{
                 &galaxy, &modes, &freeze, &size, &decay, &predelay, &diffusion, &damping, &bassCut, &lowCut, &highCut, &early, &mix,
                 &output, &modRate, &modDepth, &width, &duck, &duckRel, &shimAmount, &sync, &div, &shimPitch, &prev, &next, &preset, &save})
            addAndMakeVisible(c);
        sync.setTooltip("Sync the pre-delay to the song tempo");
        duck.setTooltip("Ducking: the reverb gets out of the way while you play and blooms in the gaps");
        early.setTooltip("Balance between the first reflections (the room's walls) and the tail");
        preset.setTooltip("Browse presets");
        save.setTooltip("Save the current settings as a preset");
    }

    void layout() {
        galaxy.setBounds(0, kTopH, getWidth(), kGalaxyBottom - kTopH);
        modes.setBounds(16, kTopH + 12, 560, 30);
        freeze.setBounds(getWidth() - 146, kTopH + 10, 130, 34);
        auto mid = juce::Rectangle<int>(getWidth() / 2 - 170, 8, 340, kTopH - 16);
        save.setBounds(mid.removeFromRight(56));
        mid.removeFromRight(6);
        prev.setBounds(mid.removeFromLeft(28));
        next.setBounds(mid.removeFromRight(28));
        preset.setBounds(mid.reduced(4, 0));

        const int y1 = kGalaxyBottom + 8, h = 132, y2 = y1 + h + 8;
        groups = {{"Space", neonCyan, {8, y1, 470, h}},     {"Tone", neonPink, {486, y1, 316, h}},
                  {"Mix", neonGold, {810, y1, 222, h}},      {"Motion", neonLime, {8, y2, 300, h}},
                  {"Dynamics", neonOrange, {316, y2, 220, h}}, {"Shimmer", neonViolet, {544, y2, 258, h}},
                  {"", line, {810, y2, 222, h}}};
        const auto knobs = [](juce::Rectangle<int> g, std::initializer_list<juce::Component*> ks) {
            auto r = g.reduced(8, 6).withTrimmedTop(24);
            const int w = r.getWidth() / static_cast<int>(ks.size());
            for (auto* k : ks) k->setBounds(r.removeFromLeft(w));
        };
        knobs(groups[0].bounds, {&size, &decay, &predelay, &diffusion});
        knobs(groups[1].bounds, {&damping, &bassCut, &lowCut, &highCut});
        knobs(groups[2].bounds, {&early, &mix, &output});
        knobs(groups[3].bounds, {&modRate, &modDepth, &width});
        knobs(groups[4].bounds, {&duck, &duckRel});
        auto sh = groups[5].bounds.reduced(8, 6).withTrimmedTop(24);
        shimPitch.setBounds(sh.removeFromLeft(110).withSizeKeepingCentre(104, 24));
        shimAmount.setBounds(sh);
        // BPM sync + division live in the Space title strip, above the pre-delay knob.
        const auto sp = groups[0].bounds;
        sync.setBounds(sp.getRight() - 196, sp.getY() + 3, 60, 20);
        div.setBounds(sp.getRight() - 132, sp.getY() + 3, 80, 20);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(bg);
        // Top bar.
        g.setColour(panel);
        g.fillRect(0, 0, getWidth(), kTopH);
        g.setColour(line);
        g.drawHorizontalLine(kTopH - 1, 0.0f, static_cast<float>(getWidth()));
        juce::Path orbit;
        orbit.addEllipse(16.0f, 15.0f, 26.0f, 18.0f);
        g.setColour(neonPink);
        g.strokePath(orbit, juce::PathStrokeType(1.6f));
        g.setColour(neonCyan);
        g.fillEllipse(25.0f, 20.0f, 8.0f, 8.0f);
        g.setColour(text);
        g.setFont(font(19.0f, true));
        g.drawText("PatVerb", 50, 0, 120, kTopH, juce::Justification::centredLeft);
        // In/out meters.
        const auto meter = [&](float x, float level, const juce::String& label) {
            const auto r = juce::Rectangle<float>(x, 14.0f, 70.0f, 6.0f);
            g.setColour(hover);
            g.fillRoundedRectangle(r, 3.0f);
            const float db = gainToDb(level), t = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 54.0f);
            g.setColour(db > -0.5f ? warn : neonCyan);
            g.fillRoundedRectangle(r.withWidth(r.getWidth() * t), 3.0f);
            g.setColour(dim);
            g.setFont(font(10.0f, true));
            g.drawText(label, juce::Rectangle<float>(x, 22.0f, 70.0f, 14.0f), juce::Justification::centredLeft);
        };
        meter(static_cast<float>(getWidth()) - 170.0f, inLevel, "IN");
        meter(static_cast<float>(getWidth()) - 90.0f, outLevel, "OUT");

        for (const auto& gr : groups) {
            drawPanel(g, gr.bounds);
            if (gr.title.isEmpty()) continue;
            g.setColour(gr.colour);
            g.fillRoundedRectangle(static_cast<float>(gr.bounds.getX() + 10), static_cast<float>(gr.bounds.getY() + 10), 3.0f, 10.0f, 1.5f);
            g.setFont(font(11.5f, true));
            g.setColour(dim);
            g.drawText(gr.title.toUpperCase(), gr.bounds.getX() + 18, gr.bounds.getY() + 4, 150, 22, juce::Justification::centredLeft);
        }
        const auto& tip = groups.back().bounds;
        g.setColour(faint);
        g.setFont(font(12.0f));
        g.drawFittedText("Drag the galaxy: left/right = size, up/down = decay.\nEvery hit you play becomes light.", tip.reduced(14, 10),
                         juce::Justification::centredLeft, 4);
    }

    void tick() {
        modes.tick();
        const bool shimmer = juce::roundToInt(proc.value(P::Mode)) == static_cast<int>(Mode::Shimmer);
        shimPitch.setEnabled(shimmer);
        shimAmount.setEnabled(shimmer);
        const bool synced = proc.value(P::Sync) > 0.5f;
        predelay.setEnabled(!synced);
        div.setEnabled(synced);
        const auto name = proc.presets().currentName();
        if (preset.getButtonText() != name) preset.setButtonText(name);
        const float in = proc.meters.inPeak.load(), out = proc.meters.outPeak.load();
        inLevel = juce::jmax(in, inLevel * 0.85f);
        outLevel = juce::jmax(out, outLevel * 0.85f);
        repaint(getWidth() - 180, 0, 180, kTopH);
    }

    PatVerbProcessor& proc;
    Galaxy galaxy;
    ModeBar modes;
    FreezeButton freeze;
    NeonKnob size, decay, predelay, diffusion, damping, bassCut, lowCut, highCut, early, mix, output, modRate, modDepth, width, duck,
        duckRel, shimAmount;
    ParamToggle sync;
    ParamCombo div, shimPitch;
    juce::TextButton prev{"<"}, next{">"}, preset, save{"SAVE"};
    std::vector<Group> groups;
    float inLevel = 0, outLevel = 0;
};

PatVerbEditor::PatVerbEditor(PatVerbProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
    setLookAndFeel(&lnf_);
    content_ = std::make_unique<Content>(p);
    addAndMakeVisible(*content_);
    content_->prev.onClick = [this] { proc_.presets().step(-1); };
    content_->next.onClick = [this] { proc_.presets().step(1); };
    content_->preset.onClick = [this] { showPresetMenu(); };
    content_->save.onClick = [this] { savePreset(); };
    setResizable(true, true);
    setResizeLimits(kWidth * 6 / 10, kHeight * 6 / 10, kWidth * 2, kHeight * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio(static_cast<double>(kWidth) / kHeight);
    setSize(kWidth, kHeight);
    startTimerHz(30);
    if (juce::SystemStats::getEnvironmentVariable("PATVERB_SNAPSHOT", {}).isNotEmpty()) snapshotCountdown_ = 20;
    pat::ui::releaseKeyboard(*this);  // keep the host's computer-keyboard MIDI working
}

PatVerbEditor::~PatVerbEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void PatVerbEditor::paint(juce::Graphics& g) { g.fillAll(bg); }

void PatVerbEditor::resized() {
    content_->setTransform(juce::AffineTransform::scale(static_cast<float>(getWidth()) / kWidth));
    content_->setBounds(0, 0, kWidth, kHeight);
    content_->layout();
}

void PatVerbEditor::timerCallback() {
    // A menu or dialog is open: let it have the message thread, refresh at about 5 fps.
    if (auto* mcm = juce::ModalComponentManager::getInstanceWithoutCreating(); mcm != nullptr && mcm->getNumModalComponents() > 0 && snapshotCountdown_ <= 0)
        if (++menuSkip_ % 6 != 0) return;
    content_->tick();
    if (snapshotCountdown_ > 0 && --snapshotCountdown_ == 0) runSnapshot();
}

void PatVerbEditor::showPresetMenu() {
    auto& pm = proc_.presets();
    pm.rescan();
    struct Node {
        std::map<juce::String, Node> kids;
        juce::StringArray order;
        juce::Array<int> items;
    };
    Node root;
    const auto& e = pm.entries();
    for (int i = 0; i < static_cast<int>(e.size()); ++i) {
        if (e[static_cast<std::size_t>(i)].category == "Init") continue;
        Node* n = &root;
        for (const auto& part : juce::StringArray::fromTokens(e[static_cast<std::size_t>(i)].category, "/", "")) {
            if (!n->kids.count(part)) n->order.add(part);
            n = &n->kids[part];
        }
        n->items.add(i);
    }
    std::function<juce::PopupMenu(const Node&)> build = [&](const Node& n) {
        juce::PopupMenu m;
        for (const auto& k : n.order) m.addSubMenu(k, build(n.kids.at(k)));
        for (int i : n.items) m.addItem(1 + i, e[static_cast<std::size_t>(i)].name, true, i == pm.currentIndex());
        return m;
    };
    juce::PopupMenu menu;
    menu.addItem(1, "Init", true, pm.currentIndex() == 0);
    menu.addSeparator();
    for (const auto& k : root.order) menu.addSubMenu(k, build(root.kids.at(k)));
    menu.addSeparator();
    menu.addItem(10001, "Save as...");
    menu.addItem(10002, "Open presets folder");
    if (pm.currentIndex() >= pm.numFactory()) menu.addItem(10003, "Delete \"" + pm.currentName() + "\"");
    juce::Component::SafePointer<PatVerbEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&content_->preset), [safe](int r) {
        if (!safe || r == 0) return;
        auto& p = safe->proc_.presets();
        if (r == 10001) safe->savePreset();
        else if (r == 10002) {
            Presets::userDirectory().createDirectory();
            Presets::userDirectory().startAsProcess();
        } else if (r == 10003) p.deleteUser(p.currentIndex());
        else p.load(r - 1);
    });
}

void PatVerbEditor::savePreset() {
    auto* w = new juce::AlertWindow("Save preset", "Name for the new preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc_.presets().currentName());
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PatVerbEditor> safe(this);
    w->enterModalState(true, juce::ModalCallbackFunction::create([safe, w](int result) {
        if (safe && result == 1 && !safe->proc_.presets().saveUser(w->getTextEditorContents("name")))
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Save preset", "Could not save the preset.");
    }), true);
}

// Diagnostic mode: PATVERB_SNAPSHOT=<folder> renders the editor with a simulated input to PNG.
// Optional PATVERB_SNAPSHOT_PRESET=<index>, PATVERB_SNAPSHOT_QUIT=1.
void PatVerbEditor::runSnapshot() {
    const juce::File dir(juce::SystemStats::getEnvironmentVariable("PATVERB_SNAPSHOT", {}));
    dir.createDirectory();
    const auto idx = juce::SystemStats::getEnvironmentVariable("PATVERB_SNAPSHOT_PRESET", "");
    if (idx.isNotEmpty()) proc_.presets().load(idx.getIntValue());
    content_->tick();
    content_->galaxy.simulate(3.0f);
    const auto img = content_->createComponentSnapshot(content_->getLocalBounds(), true, 1.0f);
    juce::PNGImageFormat png;
    const auto file = dir.getChildFile("patverb.png");
    file.deleteFile();
    if (auto out = file.createOutputStream()) png.writeImageToStream(img, *out);
    dir.getChildFile("diagnostics.txt").replaceWithText("presets=" + juce::String(static_cast<int>(proc_.presets().entries().size())) +
                                                       "\ncurrent=" + proc_.presets().currentName() + "\n");
    if (juce::SystemStats::getEnvironmentVariable("PATVERB_SNAPSHOT_QUIT", {}) == "1")
        if (auto* app = juce::JUCEApplicationBase::getInstance()) app->systemRequestedQuit();
}
