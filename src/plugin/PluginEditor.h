// PatVerb editor: the galaxy on top, neon control groups below. Fixed 1040 x 700 canvas scaled
// to the window size.
#pragma once

#include "plugin/PluginProcessor.h"
#include "plugin/ui/Controls.h"
#include "plugin/ui/Galaxy.h"

#include <memory>
#include <vector>

class PatVerbEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    static constexpr int kWidth = 1040, kHeight = 700;
    explicit PatVerbEditor(PatVerbProcessor&);
    ~PatVerbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override { return false; }  // keys belong to the host

private:
    class Content;
    void timerCallback() override;
    void showPresetMenu();
    void savePreset();
    void runSnapshot();

    PatVerbProcessor& proc_;
    ui::LookAndFeel lnf_;
    std::unique_ptr<Content> content_;
    juce::TooltipWindow tooltips_{this, 600};
    int snapshotCountdown_ = -1;
    int menuSkip_ = 0;  // throttles the refresh while a menu is open
};
