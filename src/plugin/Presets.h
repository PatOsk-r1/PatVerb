// Factory presets (in code) and user presets (.pvpreset in Documents/PatVerb/Presets, sub-folders
// become sub-categories).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

class PatVerbProcessor;

class Presets {
public:
    struct Entry {
        juce::String name, category;  // category may be a path: "Producers/Pat Osk"
        bool factory = true;
        juce::File file;
        int embedded = -1;  // index in the embedded shared-preset data
    };

    explicit Presets(PatVerbProcessor& p);
    void rescan();
    const std::vector<Entry>& entries() const noexcept { return entries_; }
    int numFactory() const noexcept { return numFactory_; }
    juce::String name(int i) const;
    bool load(int i);
    void step(int delta);
    bool saveUser(const juce::String& name);
    bool deleteUser(int i);
    int currentIndex() const noexcept { return current_; }
    juce::String currentName() const { return currentName_; }
    void setCurrentName(const juce::String& n);
    static juce::File userDirectory();

private:
    PatVerbProcessor& proc_;
    std::vector<Entry> entries_;
    int numFactory_ = 0, numCode_ = 0, current_ = 0;
    juce::String currentName_ = "Init";
};
