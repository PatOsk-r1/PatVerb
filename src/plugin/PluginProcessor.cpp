#include "plugin/PluginProcessor.h"

#include "plugin/PluginEditor.h"
#include "plugin/Presets.h"

using namespace pv;

juce::String formatParam(P p, float v) {
    const ParamInfo& pi = info(p);
    switch (pi.kind) {
        case Kind::Bool: return v > 0.5f ? "On" : "Off";
        case Kind::Choice: return juce::String(std::string(choiceName(pi.choices, juce::roundToInt(v))));
        case Kind::Float: break;
    }
    const std::string_view u = pi.unit;
    if (u == "s") return v < 1.0f ? juce::String(juce::roundToInt(v * 1000.0f)) + " ms" : juce::String(v, v < 10 ? 2 : 1) + " s";
    if (u == "ms") return juce::String(juce::roundToInt(v)) + " ms";
    if (u == "Hz") return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(v, v < 10 ? 2 : 0) + " Hz";
    if (u == "dB") return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
    return juce::String(juce::roundToInt(v * 100.0f)) + " %";
}

juce::AudioProcessorValueTreeState::ParameterLayout PatVerbProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < kNumParams; ++i) {
        const ParamInfo& pi = info(i);
        const juce::ParameterID id{juce::String(std::string(pi.key)), 1};
        const juce::String name(std::string(pi.name));
        const P p = static_cast<P>(i);
        switch (pi.kind) {
            case Kind::Float:
                layout.add(std::make_unique<juce::AudioParameterFloat>(
                    id, name, juce::NormalisableRange<float>(pi.min, pi.max, 0.0f, pi.skew), pi.def,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction([p](float v, int) { return formatParam(p, v); })));
                break;
            case Kind::Bool: layout.add(std::make_unique<juce::AudioParameterBool>(id, name, pi.def > 0.5f)); break;
            case Kind::Choice: {
                juce::StringArray items;
                for (int c = 0; c < numChoices(pi.choices); ++c) items.add(juce::String(std::string(choiceName(pi.choices, c))));
                layout.add(std::make_unique<juce::AudioParameterChoice>(id, name, items, static_cast<int>(pi.def)));
                break;
            }
        }
    }
    return layout;
}

PatVerbProcessor::PatVerbProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout()) {
    for (int i = 0; i < kNumParams; ++i) {
        const juce::String key(std::string(info(i).key));
        params_[static_cast<std::size_t>(i)] = apvts.getParameter(key);
        raw_[static_cast<std::size_t>(i)] = apvts.getRawParameterValue(key);
    }
    presets_ = std::make_unique<Presets>(*this);
}

PatVerbProcessor::~PatVerbProcessor() = default;

bool PatVerbProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void PatVerbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    reverb_.prepare(sampleRate, samplesPerBlock);
    scratch_.setSize(2, juce::jmax(1, samplesPerBlock));
}

void PatVerbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    for (int i = 0; i < kNumParams; ++i) values_[static_cast<std::size_t>(i)] = raw_[static_cast<std::size_t>(i)]->load(std::memory_order_relaxed);
    double bpm = 120;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    reverb_.setParams(values_.data(), bpm);

    // Work in stereo whatever the host layout is.
    if (scratch_.getNumSamples() < n) scratch_.setSize(2, n, false, false, true);
    float* l = scratch_.getWritePointer(0);
    float* r = scratch_.getWritePointer(1);
    juce::FloatVectorOperations::copy(l, buffer.getReadPointer(0), n);
    juce::FloatVectorOperations::copy(r, buffer.getReadPointer(ins > 1 ? 1 : 0), n);
    reverb_.process(l, r, n);
    if (outs > 1) {
        buffer.copyFrom(0, 0, l, n);
        buffer.copyFrom(1, 0, r, n);
    } else {
        for (int i = 0; i < n; ++i) buffer.getWritePointer(0)[i] = 0.5f * (l[i] + r[i]);
    }
    for (int c = 2; c < outs; ++c) buffer.clear(c, 0, n);

    const auto& m = reverb_.meters();
    meters.low.store(m.low, std::memory_order_relaxed);
    meters.mid.store(m.mid, std::memory_order_relaxed);
    meters.high.store(m.high, std::memory_order_relaxed);
    meters.wet.store(m.wet, std::memory_order_relaxed);
    meters.inPeak.store(m.inPeak, std::memory_order_relaxed);
    meters.outPeak.store(m.outPeak, std::memory_order_relaxed);
    meters.onsets.store(m.onsets, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* PatVerbProcessor::createEditor() { return new PatVerbEditor(*this); }

void PatVerbProcessor::setValue(P p, float v) {
    if (auto* prm = param(p)) prm->setValueNotifyingHost(prm->convertTo0to1(juce::jlimit(info(p).min, info(p).max, v)));
}

juce::ValueTree PatVerbProcessor::createPatch() const {
    juce::ValueTree patch("PatVerbPreset");
    patch.setProperty("version", 1, nullptr);
    for (int i = 0; i < kNumParams; ++i)
        patch.setProperty(juce::Identifier(std::string(info(i).key)), raw_[static_cast<std::size_t>(i)]->load(), nullptr);
    return patch;
}

void PatVerbProcessor::loadPatch(const juce::ValueTree& patch) {
    if (!patch.hasType("PatVerbPreset")) return;
    for (int i = 0; i < kNumParams; ++i) {
        const juce::Identifier key(std::string(info(i).key));
        const float v = patch.hasProperty(key) ? static_cast<float>(patch.getProperty(key)) : info(i).def;
        setValue(static_cast<P>(i), v);
    }
}

void PatVerbProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto patch = createPatch();
    patch.setProperty("preset", presets_->currentName(), nullptr);
    if (auto xml = patch.createXml()) copyXmlToBinary(*xml, dest);
}

void PatVerbProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        const auto patch = juce::ValueTree::fromXml(*xml);
        loadPatch(patch);
        presets_->setCurrentName(patch.getProperty("preset", "Init").toString());
    }
}

int PatVerbProcessor::getNumPrograms() { return juce::jmax(1, presets_->numFactory()); }
int PatVerbProcessor::getCurrentProgram() { return juce::jmax(0, presets_->currentIndex()); }
void PatVerbProcessor::setCurrentProgram(int i) { presets_->load(i); }
const juce::String PatVerbProcessor::getProgramName(int i) { return presets_->name(i); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PatVerbProcessor(); }
