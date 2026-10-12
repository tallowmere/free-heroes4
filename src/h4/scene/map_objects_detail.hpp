#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "h4/core/geometry.hpp"

namespace h4::formats {
class BitmapGroup;
class ScenarioGrid;
struct SubImageLayers;
}

namespace h4::scene::detail {

inline constexpr core::Point kCellCentre{0, 16};

inline constexpr std::array<std::string_view, 7>
    kFlagAnimations{"flags.red", "flags.blue", "flags.green", "flags.orange", "flags.purple", "flags.teal", "flags.gray"};

inline constexpr std::string_view kSelectionMarker = "internal.selected.army";
inline constexpr std::uint32_t kSelectionOrder     = 0xFFFFFFFFU;

[[nodiscard]] std::int32_t army_height(const formats::ScenarioGrid& grid, core::Point cell, std::int32_t level);

}
