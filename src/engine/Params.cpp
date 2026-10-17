#include "engine/Params.h"

namespace pv {
namespace {

#define PV_INFO(id, key, name, kind, mn, mx, def, skew, ch, unit) \
    ParamInfo{key, name, Kind::kind, float(mn), float(mx), float(def), float(skew), Choices::ch, unit},
const std::array<ParamInfo, kNumParams> kTable{{PV_PARAM_LIST(PV_INFO)}};
#undef PV_INFO

constexpr std::string_view kModes[] = {"Hall", "Room", "Chamber", "Plate", "Spring", "Shimmer", "Cloud"};
constexpr std::string_view kDivs[] = {"1/64", "1/32", "1/16 T", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4", "1/4 D"};
constexpr double kDivBeats[] = {1.0 / 16, 1.0 / 8, 1.0 / 6, 0.25, 0.375, 1.0 / 3, 0.5, 0.75, 1.0, 1.5};
constexpr std::string_view kShim[] = {"+12 st", "+7 st", "+5 st", "+19 st", "+24 st", "-12 st"};
constexpr float kShimSemis[] = {12, 7, 5, 19, 24, -12};

}  // namespace

const ParamInfo& info(P p) noexcept { return kTable[static_cast<std::size_t>(p)]; }

int findParam(std::string_view key) noexcept {
    for (int i = 0; i < kNumParams; ++i)
        if (kTable[static_cast<std::size_t>(i)].key == key) return i;
    return -1;
}

int numChoices(Choices c) noexcept {
    switch (c) {
        case Choices::Mode: return static_cast<int>(std::size(kModes));
        case Choices::Div: return static_cast<int>(std::size(kDivs));
        case Choices::Shimmer: return static_cast<int>(std::size(kShim));
        case Choices::None: break;
    }
    return 0;
}

std::string_view choiceName(Choices c, int i) noexcept {
    if (i < 0 || i >= numChoices(c)) return {};
    switch (c) {
        case Choices::Mode: return kModes[i];
        case Choices::Div: return kDivs[i];
        case Choices::Shimmer: return kShim[i];
        case Choices::None: break;
    }
    return {};
}

double divisionBeats(int i) noexcept { return kDivBeats[i < 0 ? 0 : (i > 9 ? 9 : i)]; }
float shimmerSemitones(int i) noexcept { return kShimSemis[i < 0 ? 0 : (i > 5 ? 5 : i)]; }

}  // namespace pv
