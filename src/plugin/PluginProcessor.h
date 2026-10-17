// PatVerb as a JUCE effect plugin: host parameters built from engine/Params.h, the reverb, and
// lock-free meters the galaxy visual reads.
#pragma once

#include "engine/Reverb.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <memory>

class Presets;

juce::String formatParam(pv::P p, float value);

class PatVerbProcessor final : public juce::AudioProcessor {
public:
    PatVerbProcessor();
    ~PatVerbProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "PatVerb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    Presets& presets() noexcept { return *presets_; }
    juce::RangedAudioParameter* param(pv::P p) const noexcept { return params_[static_cast<std::size_t>(p)]; }
    float value(pv::P p) const noexcept { return raw_[static_cast<std::size_t>(p)]->load(); }
    void setValue(pv::P p, float v);  // from the UI, with host notification

    juce::ValueTree createPatch() const;
    void loadPatch(const juce::ValueTree& patch);

    struct Meters {
        std::atomic<float> low{0}, mid{0}, high{0}, wet{0}, inPeak{0}, outPeak{0};
        std::atomic<int> onsets{0};
    } meters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    pv::Reverb reverb_;
    std::unique_ptr<Presets> presets_;
    std::array<juce::RangedAudioParameter*, pv::kNumParams> params_{};
    std::array<std::atomic<float>*, pv::kNumParams> raw_{};
    std::array<float, pv::kNumParams> values_{};
    juce::AudioBuffer<float> scratch_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatVerbProcessor)
};
