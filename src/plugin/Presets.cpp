#include "plugin/Presets.h"

#include "plugin/PluginProcessor.h"

#include <PresetData.h>

#include <initializer_list>
#include <utility>

using namespace pv;

namespace {

constexpr float Hall = 0, Room = 1, Chamber = 2, Plate = 3, Spring = 4, Shimmer = 5, Cloud = 6;

struct Factory {
    const char* name;
    const char* category;
    std::initializer_list<std::pair<P, float>> values;  // anything left out keeps its default
};

// Divisions: 0 1/64, 1 1/32, 2 1/16T, 3 1/16, 4 1/16D, 5 1/8T, 6 1/8, 7 1/8D, 8 1/4, 9 1/4D.
// Shimmer pitch: 0 +12, 1 +7, 2 +5, 3 +19, 4 +24, 5 -12.
const Factory kFactory[] = {
    {"Init", "Init", {}},

    {"Concert Hall", "Halls", {{P::Mode, Hall}, {P::Size, 0.75f}, {P::Decay, 3.2f}, {P::PreDelay, 30.0f}, {P::Mix, 0.3f}}},
    {"Cathedral", "Halls", {{P::Mode, Hall}, {P::Size, 1.0f}, {P::Decay, 9.0f}, {P::PreDelay, 60.0f}, {P::Damping, 5000.0f}, {P::Mix, 0.35f}, {P::ModDepth, 0.4f}}},
    {"Bright Hall", "Halls", {{P::Mode, Hall}, {P::Size, 0.6f}, {P::Decay, 2.2f}, {P::Damping, 14000.0f}, {P::Early, 0.25f}, {P::Mix, 0.25f}}},
    {"Ducked Lead Hall", "Halls", {{P::Mode, Hall}, {P::Size, 0.7f}, {P::Decay, 3.5f}, {P::Duck, 0.7f}, {P::DuckRelease, 300.0f}, {P::Mix, 0.4f}}},

    {"Drum Room", "Rooms", {{P::Mode, Room}, {P::Size, 0.4f}, {P::Decay, 0.7f}, {P::Early, 0.7f}, {P::Damping, 9000.0f}, {P::Mix, 0.25f}}},
    {"Studio Booth", "Rooms", {{P::Mode, Room}, {P::Size, 0.15f}, {P::Decay, 0.35f}, {P::Early, 0.8f}, {P::Mix, 0.2f}}},
    {"Lo-Fi Room", "Rooms", {{P::Mode, Room}, {P::Size, 0.35f}, {P::Decay, 0.9f}, {P::LowCut, 300.0f}, {P::HighCut, 4000.0f}, {P::Mix, 0.35f}}},
    {"Small Chamber", "Rooms", {{P::Mode, Chamber}, {P::Size, 0.45f}, {P::Decay, 1.4f}, {P::Early, 0.5f}, {P::Mix, 0.3f}}},

    {"Vocal Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.55f}, {P::Decay, 1.8f}, {P::PreDelay, 25.0f}, {P::LowCut, 150.0f}, {P::Mix, 0.25f}}},
    {"Snare Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.4f}, {P::Decay, 1.2f}, {P::Damping, 11000.0f}, {P::Mix, 0.3f}}},
    {"Dark Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.7f}, {P::Decay, 3.0f}, {P::Damping, 3500.0f}, {P::Mix, 0.3f}}},

    {"Surf Spring", "Springs", {{P::Mode, Spring}, {P::Size, 0.5f}, {P::Decay, 2.2f}, {P::Mix, 0.35f}}},
    {"Dub Spring", "Springs", {{P::Mode, Spring}, {P::Size, 0.8f}, {P::Decay, 4.0f}, {P::LowCut, 120.0f}, {P::Mix, 0.45f}}},

    {"Shimmer Heaven", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.9f}, {P::Decay, 7.0f}, {P::ShimPitch, 0.0f}, {P::ShimAmount, 0.55f}, {P::Mix, 0.4f}}},
    {"Shimmer Fifths", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.8f}, {P::Decay, 5.0f}, {P::ShimPitch, 1.0f}, {P::ShimAmount, 0.45f}, {P::Mix, 0.35f}}},
    {"Sub Shimmer", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.85f}, {P::Decay, 6.0f}, {P::ShimPitch, 5.0f}, {P::ShimAmount, 0.4f}, {P::HighCut, 6000.0f}, {P::Mix, 0.35f}}},

    {"Infinite Cloud", "Ambient", {{P::Mode, Cloud}, {P::Size, 1.0f}, {P::Decay, 20.0f}, {P::ModDepth, 0.6f}, {P::Mix, 0.45f}}},
    {"Ambient Wash", "Ambient", {{P::Mode, Cloud}, {P::Size, 0.8f}, {P::Decay, 8.0f}, {P::Damping, 6000.0f}, {P::Mix, 0.4f}}},
    {"Slow Galaxy", "Ambient", {{P::Mode, Cloud}, {P::Size, 0.95f}, {P::Decay, 12.0f}, {P::ModRate, 0.15f}, {P::ModDepth, 0.8f}, {P::Width, 1.4f}, {P::Mix, 0.5f}}},

    // Producers
    {"Vestuario del Centenario", "Producers/Torbey Beats", {{P::Mode, Plate}, {P::Sync, 1.0f}, {P::Div, 3.0f}, {P::Decay, 1.6f}, {P::Duck, 0.5f}, {P::LowCut, 250.0f}, {P::Mix, 0.3f}}},
    {"Catedral de Montevideo 4 AM", "Producers/Torbey Beats", {{P::Mode, Hall}, {P::Size, 0.85f}, {P::Decay, 4.5f}, {P::Damping, 4500.0f}, {P::Sync, 1.0f}, {P::Div, 6.0f}, {P::Mix, 0.35f}}},
    {"Niebla en Punta del Este", "Producers/Torbey Beats", {{P::Mode, Cloud}, {P::Size, 0.9f}, {P::Decay, 9.0f}, {P::LowCut, 200.0f}, {P::Mix, 0.45f}}},
    {"Sala de Ensayo en Cordón", "Producers/Pat Osk", {{P::Mode, Chamber}, {P::Size, 0.35f}, {P::Decay, 0.9f}, {P::Early, 0.6f}, {P::Damping, 12000.0f}, {P::Mix, 0.3f}}},
    {"Cola del Río de la Plata", "Producers/Pat Osk", {{P::Mode, Shimmer}, {P::Size, 0.8f}, {P::Decay, 5.5f}, {P::ShimPitch, 0.0f}, {P::ShimAmount, 0.3f}, {P::Duck, 0.4f}, {P::Mix, 0.35f}}},
    {"Resorte del Ómnibus Viejo", "Producers/Pat Osk", {{P::Mode, Spring}, {P::Size, 0.3f}, {P::Decay, 1.5f}, {P::ModDepth, 0.9f}, {P::ModRate, 3.0f}, {P::Mix, 0.4f}}},
    // Second batch
    {"Opera House", "Halls", {{P::Mode, Hall}, {P::Size, 0.9f}, {P::Decay, 4.5f}, {P::PreDelay, 45.0f}, {P::Damping, 8000.0f}, {P::Early, 0.3f}, {P::Mix, 0.3f}}},
    {"Orchestral Stage", "Halls", {{P::Mode, Hall}, {P::Size, 0.8f}, {P::Decay, 2.8f}, {P::PreDelay, 20.0f}, {P::Early, 0.55f}, {P::Width, 1.3f}, {P::Mix, 0.28f}}},
    {"Gothic Church", "Halls", {{P::Mode, Hall}, {P::Size, 1.0f}, {P::Decay, 12.0f}, {P::PreDelay, 80.0f}, {P::Damping, 3500.0f}, {P::BassCut, 120.0f}, {P::Mix, 0.35f}}},
    {"Piano Hall", "Halls", {{P::Mode, Hall}, {P::Size, 0.65f}, {P::Decay, 2.4f}, {P::PreDelay, 15.0f}, {P::Damping, 9000.0f}, {P::Mix, 0.22f}}},
    {"Tight Ambience", "Rooms", {{P::Mode, Room}, {P::Size, 0.2f}, {P::Decay, 0.25f}, {P::Early, 0.9f}, {P::Mix, 0.18f}}},
    {"Live Room", "Rooms", {{P::Mode, Room}, {P::Size, 0.5f}, {P::Decay, 1.1f}, {P::Early, 0.6f}, {P::Damping, 10000.0f}, {P::Mix, 0.28f}}},
    {"Garage", "Rooms", {{P::Mode, Room}, {P::Size, 0.55f}, {P::Decay, 1.6f}, {P::Early, 0.75f}, {P::Damping, 5000.0f}, {P::Mix, 0.3f}}},
    {"Bathroom Tiles", "Rooms", {{P::Mode, Room}, {P::Size, 0.25f}, {P::Decay, 1.2f}, {P::Early, 0.85f}, {P::Damping, 15000.0f}, {P::Diffusion, 0.5f}, {P::Mix, 0.35f}}},
    {"Wood Chamber", "Rooms", {{P::Mode, Chamber}, {P::Size, 0.5f}, {P::Decay, 1.8f}, {P::Damping, 6000.0f}, {P::Early, 0.45f}, {P::Mix, 0.3f}}},
    {"Echo Chamber", "Rooms", {{P::Mode, Chamber}, {P::Size, 0.7f}, {P::Decay, 2.6f}, {P::Diffusion, 0.6f}, {P::Early, 0.4f}, {P::Mix, 0.32f}}},
    {"Vintage Gold Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.6f}, {P::Decay, 2.4f}, {P::PreDelay, 10.0f}, {P::Damping, 8000.0f}, {P::LowCut, 120.0f}, {P::Mix, 0.28f}}},
    {"Rap Vocal Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.45f}, {P::Decay, 1.0f}, {P::PreDelay, 40.0f}, {P::LowCut, 250.0f}, {P::Duck, 0.6f}, {P::Mix, 0.22f}}},
    {"Synced 1/8 Plate", "Plates", {{P::Mode, Plate}, {P::Sync, 1.0f}, {P::Div, 6.0f}, {P::Decay, 1.5f}, {P::LowCut, 200.0f}, {P::Mix, 0.3f}}},
    {"Bright Long Plate", "Plates", {{P::Mode, Plate}, {P::Size, 0.85f}, {P::Decay, 4.5f}, {P::Damping, 16000.0f}, {P::Mix, 0.3f}}},
    {"Guitar Amp Spring", "Springs", {{P::Mode, Spring}, {P::Size, 0.4f}, {P::Decay, 1.6f}, {P::LowCut, 150.0f}, {P::HighCut, 7000.0f}, {P::Mix, 0.3f}}},
    {"Boing Spring", "Springs", {{P::Mode, Spring}, {P::Size, 0.65f}, {P::Decay, 3.0f}, {P::ModDepth, 0.6f}, {P::Mix, 0.4f}}},
    {"Octave Up Shimmer", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.85f}, {P::Decay, 6.0f}, {P::ShimPitch, 4.0f}, {P::ShimAmount, 0.35f}, {P::Mix, 0.35f}}},
    {"Fourth Shimmer Pad", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.8f}, {P::Decay, 5.5f}, {P::ShimPitch, 2.0f}, {P::ShimAmount, 0.45f}, {P::LowCut, 200.0f}, {P::Mix, 0.35f}}},
    {"Crystal Twelfth", "Shimmer", {{P::Mode, Shimmer}, {P::Size, 0.9f}, {P::Decay, 8.0f}, {P::ShimPitch, 3.0f}, {P::ShimAmount, 0.35f}, {P::HighCut, 12000.0f}, {P::Mix, 0.35f}}},
    {"Endless Pad", "Ambient", {{P::Mode, Cloud}, {P::Size, 1.0f}, {P::Decay, 28.0f}, {P::ModDepth, 0.5f}, {P::Mix, 0.5f}}},
    {"Dreamy Swells", "Ambient", {{P::Mode, Cloud}, {P::Size, 0.85f}, {P::Decay, 10.0f}, {P::PreDelay, 120.0f}, {P::ModDepth, 0.5f}, {P::Duck, 0.5f}, {P::Mix, 0.45f}}},
    {"Underwater Cave", "Ambient", {{P::Mode, Cloud}, {P::Size, 0.9f}, {P::Decay, 7.0f}, {P::HighCut, 2000.0f}, {P::ModDepth, 0.7f}, {P::Mix, 0.45f}}},
    {"Reverse-ish Swell", "Ambient", {{P::Mode, Cloud}, {P::Size, 0.7f}, {P::Decay, 4.0f}, {P::PreDelay, 250.0f}, {P::Early, 0.0f}, {P::Mix, 0.4f}}},
    {"Kick Tail Short", "Drums", {{P::Mode, Room}, {P::Size, 0.3f}, {P::Decay, 0.5f}, {P::LowCut, 80.0f}, {P::HighCut, 6000.0f}, {P::Mix, 0.15f}}},
    {"Snare Big 80s", "Drums", {{P::Mode, Plate}, {P::Size, 0.75f}, {P::Decay, 2.0f}, {P::PreDelay, 5.0f}, {P::Damping, 12000.0f}, {P::Mix, 0.35f}}},
    {"Clap Room", "Drums", {{P::Mode, Room}, {P::Size, 0.45f}, {P::Decay, 0.8f}, {P::Early, 0.8f}, {P::LowCut, 300.0f}, {P::Mix, 0.3f}}},
    {"Hi-Hat Air", "Drums", {{P::Mode, Plate}, {P::Size, 0.3f}, {P::Decay, 0.6f}, {P::LowCut, 2000.0f}, {P::Mix, 0.2f}}},

    {"Pasillo del Liceo", "Producers/Torbey Beats", {{P::Mode, Room}, {P::Size, 0.6f}, {P::Decay, 1.9f}, {P::Early, 0.7f}, {P::Damping, 8000.0f}, {P::Mix, 0.3f}}},
    {"Túnel de Avenida Italia", "Producers/Torbey Beats", {{P::Mode, Chamber}, {P::Size, 0.85f}, {P::Decay, 3.5f}, {P::Early, 0.6f}, {P::Damping, 4000.0f}, {P::Mix, 0.35f}}},
    {"Tablado de Carnaval", "Producers/Torbey Beats", {{P::Mode, Hall}, {P::Size, 0.55f}, {P::Decay, 1.8f}, {P::Early, 0.5f}, {P::Width, 1.3f}, {P::Mix, 0.3f}}},
    {"Charco en la Ciudad Vieja", "Producers/Torbey Beats", {{P::Mode, Spring}, {P::Size, 0.55f}, {P::Decay, 2.5f}, {P::HighCut, 6000.0f}, {P::Mix, 0.35f}}},
    {"Faro de José Ignacio", "Producers/Pat Osk", {{P::Mode, Shimmer}, {P::Size, 0.95f}, {P::Decay, 9.0f}, {P::ShimPitch, 0.0f}, {P::ShimAmount, 0.4f}, {P::LowCut, 250.0f}, {P::Mix, 0.4f}}},
    {"Mate en la Escollera", "Producers/Pat Osk", {{P::Mode, Cloud}, {P::Size, 0.8f}, {P::Decay, 6.0f}, {P::Damping, 5000.0f}, {P::ModDepth, 0.4f}, {P::Duck, 0.4f}, {P::Mix, 0.4f}}},
    {"Plaza Independencia de Noche", "Producers/Pat Osk", {{P::Mode, Hall}, {P::Size, 0.85f}, {P::Decay, 5.0f}, {P::PreDelay, 50.0f}, {P::Width, 1.4f}, {P::Mix, 0.35f}}},
    {"Garrafa en el Patio", "Producers/Pat Osk", {{P::Mode, Plate}, {P::Size, 0.35f}, {P::Decay, 0.9f}, {P::Damping, 14000.0f}, {P::Sync, 1.0f}, {P::Div, 3.0f}, {P::Mix, 0.3f}}},
};

}  // namespace

Presets::Presets(PatVerbProcessor& p) : proc_(p) { rescan(); }

juce::File Presets::userDirectory() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("PatVerb").getChildFile("Presets");
}

void Presets::rescan() {
    entries_.clear();
    for (const auto& f : kFactory) entries_.push_back({juce::String::fromUTF8(f.name), juce::String::fromUTF8(f.category), true, {}});  // names have accents
    numCode_ = static_cast<int>(entries_.size());
    const auto dir = userDirectory();
    auto files = dir.findChildFiles(juce::File::findFiles, true, "*.pvpreset");
    std::sort(files.begin(), files.end(), [&dir](const juce::File& a, const juce::File& b) {
        return a.getRelativePathFrom(dir).compareNatural(b.getRelativePathFrom(dir)) < 0;
    });
    // Shared presets built into the plugin (tools/collect-presets.ps1); the author's own copy wins.
    juce::StringArray userNames;
    for (const auto& f : files) userNames.add(f.getFileNameWithoutExtension());
    for (int i = 0; i < PresetData::namedResourceListSize; ++i) {
        const juce::String original(PresetData::originalFilenames[i]);
        if (!original.endsWithIgnoreCase(".pvpreset")) continue;
        int size = 0;
        const char* data = PresetData::getNamedResource(PresetData::namedResourceList[i], size);
        auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size));
        const auto name = original.upToLastOccurrenceOf(".", false, false);
        if (!xml || userNames.contains(name)) continue;
        Entry e{name, xml->getStringAttribute("category", "Community"), true, {}};
        e.embedded = i;
        entries_.push_back(e);
    }
    numFactory_ = static_cast<int>(entries_.size());
    for (const auto& f : files) {
        juce::String cat = "User";
        if (f.getParentDirectory() != dir) cat << "/" << f.getParentDirectory().getRelativePathFrom(dir).replaceCharacter('\\', '/');
        entries_.push_back({f.getFileNameWithoutExtension(), cat, false, f});
    }
    setCurrentName(currentName_);
}

juce::String Presets::name(int i) const {
    return (i >= 0 && i < static_cast<int>(entries_.size())) ? entries_[static_cast<std::size_t>(i)].name : juce::String();
}

bool Presets::load(int i) {
    if (i < 0 || i >= static_cast<int>(entries_.size())) return false;
    const auto& e = entries_[static_cast<std::size_t>(i)];
    juce::ValueTree patch("PatVerbPreset");
    if (e.embedded >= 0) {
        int size = 0;
        const char* data = PresetData::getNamedResource(PresetData::namedResourceList[e.embedded], size);
        if (auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size))) patch = juce::ValueTree::fromXml(*xml);
    } else if (e.factory && i < numCode_) {
        for (const auto& [p, v] : kFactory[i].values) patch.setProperty(juce::Identifier(std::string(info(p).key)), v, nullptr);
    } else if (auto xml = juce::XmlDocument::parse(e.file)) {
        patch = juce::ValueTree::fromXml(*xml);
    }
    if (!patch.hasType("PatVerbPreset")) return false;
    proc_.loadPatch(patch);
    current_ = i;
    currentName_ = e.name;
    return true;
}

void Presets::step(int delta) {
    const int n = static_cast<int>(entries_.size());
    if (n > 0) load(((current_ < 0 ? 0 : current_) + delta + n) % n);
}

bool Presets::saveUser(const juce::String& rawName) {
    const auto name = juce::File::createLegalFileName(rawName.trim());
    if (name.isEmpty() || !userDirectory().createDirectory()) return false;
    auto xml = proc_.createPatch().createXml();
    if (!xml || !xml->writeTo(userDirectory().getChildFile(name + ".pvpreset"))) return false;
    currentName_ = name;
    rescan();
    return true;
}

bool Presets::deleteUser(int i) {
    if (i < numFactory_ || i >= static_cast<int>(entries_.size())) return false;
    const bool ok = entries_[static_cast<std::size_t>(i)].file.moveToTrash();
    rescan();
    return ok;
}

void Presets::setCurrentName(const juce::String& n) {
    currentName_ = n;
    current_ = -1;
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
        if (entries_[static_cast<std::size_t>(i)].name == n) {
            current_ = i;
            break;
        }
}
