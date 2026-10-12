#pragma once

#include <algorithm>
#include <cstdint>

#include "h4/core/geometry.hpp"

namespace h4::graphics {

struct CornerLifts {
    std::int32_t left   = 0;
    std::int32_t top    = 0;
    std::int32_t right  = 0;
    std::int32_t bottom = 0;
};

inline constexpr std::int32_t kDiamondWidth  = 64;
inline constexpr std::int32_t kDiamondHeight = 32;

[[nodiscard]] constexpr std::int32_t half_column_lerp(std::int32_t from, std::int32_t to, std::int32_t t) noexcept {
    return from + (to - from) * t / 32;
}

template <class PlotColumn>
constexpr void for_each_diamond_column(core::Point top, CornerLifts lifts, const core::Rect& clip, PlotColumn plot) {
    const std::int32_t y_top    = top.y - lifts.top;
    const std::int32_t y_left   = top.y - lifts.left + kDiamondHeight / 2;
    const std::int32_t y_right  = top.y - lifts.right + kDiamondHeight / 2;
    const std::int32_t y_bottom = top.y - lifts.bottom + kDiamondHeight;
    if (!(top.x < clip.right() && clip.left() < top.x + kDiamondWidth))
        return;
    if (!(y_top < clip.bottom() && clip.top() < y_bottom))
        return;

    const std::int32_t half = kDiamondWidth / 2;
    for (std::int32_t c = 0; c < kDiamondWidth; ++c) {
        const std::int32_t x = top.x + c;
        if (x < clip.left() || x >= clip.right())
            continue;
        std::int32_t y_begin = 0;
        std::int32_t y_end   = 0;
        if (c < half) {
            y_begin = half_column_lerp(y_top, y_left, half - 1 - c);
            y_end   = half_column_lerp(y_left, y_bottom, c);
        } else {
            y_begin = half_column_lerp(y_top, y_right, c - half);
            y_end   = half_column_lerp(y_right, y_bottom, kDiamondWidth - 1 - c);
        }
        y_begin = std::max(y_begin, clip.top());
        y_end   = std::min(y_end, clip.bottom());
        if (y_begin < y_end)
            plot(x, y_begin, y_end);
    }
}

}
