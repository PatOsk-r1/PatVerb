// The single list of PatVerb parameters (host parameters are built from it).
// RULE: keys never change once released; presets and host projects store them.
#pragma once

#include <array>
#include <cmath>
#include <string_view>

namespace pv {

enum class Kind : unsigned char { Float, Bool, Choice };
enum class Choices : unsigned char { None, Mode, Div, Shimmer };

// X(id, key, name, kind, min, max, default, skew, choices, unit)
#define PV_PARAM_LIST(X)                                                                        \
    X(Mode, "mode", "Mode", Choice, 0, 6, 0, 1, Mode, "")                                       \
    X(Mix, "mix", "Mix", Float, 0, 1, 0.3, 1, None, "%")                                        \
    X(PreDelay, "predelay", "Pre-delay", Float, 0, 500, 20, 0.5, None, "ms")                    \
    X(Sync, "sync", "Pre-delay Sync", Bool, 0, 1, 0, 1, None, "")                               \
    X(Div, "div", "Pre-delay Division", Choice, 0, 9, 6, 1, Div, "")                            \
    X(Size, "size", "Size", Float, 0, 1, 0.6, 1, None, "%")                                     \
    X(Decay, "decay", "Decay", Float, 0.1, 30, 2.5, 0.3, None, "s")                             \
    X(Diffusion, "diffusion", "Diffusion", Float, 0, 1, 0.8, 1, None, "%")                      \
    X(Damping, "damping", "High Damping", Float, 1000, 20000, 7000, 0.4, None, "Hz")            \
    X(BassCut, "basscut", "Bass Cut", Float, 20, 1000, 60, 0.4, None, "Hz")                     \
    X(Early, "early", "Early / Late", Float, 0, 1, 0.35, 1, None, "%")                          \
    X(ModRate, "modrate", "Mod Rate", Float, 0.05, 5, 0.6, 0.5, None, "Hz")                     \
    X(ModDepth, "moddepth", "Mod Depth", Float, 0, 1, 0.3, 1, None, "%")                        \
    X(Width, "width", "Width", Float, 0, 1.5, 1, 1, None, "%")                                  \
    X(Duck, "duck", "Ducking", Float, 0, 1, 0, 1, None, "%")                                    \
    X(DuckRelease, "duckrel", "Ducking Release", Float, 20, 1000, 200, 0.5, None, "ms")         \
    X(Freeze, "freeze", "Freeze", Bool, 0, 1, 0, 1, None, "")                                   \
    X(ShimPitch, "shimpitch", "Shimmer Pitch", Choice, 0, 5, 0, 1, Shimmer, "")                 \
    X(ShimAmount, "shimamount", "Shimmer Amount", Float, 0, 1, 0.5, 1, None, "%")               \
    X(LowCut, "lowcut", "Wet Low Cut", Float, 20, 1000, 20, 0.4, None, "Hz")                    \
    X(HighCut, "highcut", "Wet High Cut", Float, 1000, 20000, 20000, 0.4, None, "Hz")           \
    X(Output, "output", "Output", Float, -24, 12, 0, 1, None, "dB")

enum class P : int {
#define PV_ENUM(id, ...) id,
    PV_PARAM_LIST(PV_ENUM)
#undef PV_ENUM
    Count
};
inline constexpr int kNumParams = static_cast<int>(P::Count);

struct ParamInfo {
    std::string_view key, name;
    Kind kind;
    float min, max, def, skew;
    Choices choices;
    std::string_view unit;
};

const ParamInfo& info(P p) noexcept;
inline const ParamInfo& info(int i) noexcept { return info(static_cast<P>(i)); }
int findParam(std::string_view key) noexcept;
int numChoices(Choices c) noexcept;
std::string_view choiceName(Choices c, int index) noexcept;
double divisionBeats(int index) noexcept;    // pre-delay divisions, in quarter notes
float shimmerSemitones(int index) noexcept;  // shimmer pitch choices

enum class Mode : int { Hall, Room, Chamber, Plate, Spring, Shimmer, Cloud };

}  // namespace pv
