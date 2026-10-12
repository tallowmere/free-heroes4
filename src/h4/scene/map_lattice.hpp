#pragma once

#include <algorithm>
#include <cstdint>

#include "h4/core/geometry.hpp"

namespace h4::formats {
class ScenarioGrid;
}

namespace h4::scene {

[[nodiscard]] core::Point tile_anchor(std::int32_t x, std::int32_t y, std::int32_t width) noexcept;
inline constexpr std::int32_t kTileWidth  = 64;
inline constexpr std::int32_t kTileHeight = 32;
inline constexpr std::int32_t kHalfTile = kTileWidth / 2;

[[nodiscard]] constexpr std::int32_t object_column(std::int32_t x) noexcept {
    return (x >= 0 ? x : x - (kHalfTile - 1)) / kHalfTile;
}

struct ColumnRange {
    std::int32_t first = 0;
    std::int32_t last  = 0;
};
[[nodiscard]] constexpr ColumnRange edge_columns(std::int32_t left, std::int32_t right, std::int32_t width) noexcept {
    return {std::max(0, left / kHalfTile), std::min(width, (right + kHalfTile - 1) / kHalfTile)};
}

[[nodiscard]] core::Point tile_pin(std::int32_t x, std::int32_t y, std::int32_t width) noexcept;

[[nodiscard]] core::Rect map_box(const formats::ScenarioGrid& grid) noexcept;

[[nodiscard]] core::Point box_anchor(std::int32_t x, std::int32_t y, std::int32_t width) noexcept;

[[nodiscard]] core::Point box_pin(core::Point cell, std::int32_t width, std::int32_t height = 0) noexcept;

inline constexpr std::int32_t kRaiseAllowance = 255;

[[nodiscard]] constexpr bool tile_reaches(core::Point anchor, const core::Rect& area) noexcept {
    return anchor.x + kTileWidth > area.left() && anchor.x < area.right() && anchor.y + kTileHeight > area.top() &&
        anchor.y - kRaiseAllowance < area.bottom();
}

}
