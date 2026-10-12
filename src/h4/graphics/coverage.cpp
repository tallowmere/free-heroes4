#include "h4/graphics/coverage.hpp"

#include <algorithm>
#include <span>

#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"

namespace h4::graphics {

namespace {

template <class Paint>
void walk_units(const CoverageSurface& surface, core::Point anchor, CornerLifts lifts, Paint paint) {
    const std::span<const std::uint8_t> widths = formats::terrain_row_widths();
    const core::Rect clip                      = surface.bounds();

    for_each_diamond_column(anchor,
                            lifts,
                            core::Rect::from_edges(clip.left(), -(1 << 24), clip.right(), 1 << 24),
                            [&](std::int32_t x, std::int32_t y_begin, std::int32_t y_end) {
                                const std::size_t column = static_cast<std::size_t>(x - anchor.x);
                                const std::int32_t count = y_end - y_begin;
                                if (count <= 0 || column >= kDiamondWidth)
                                    return;
                                const std::int32_t width = widths[column];

                                std::size_t unit = 0;
                                std::int32_t acc = 0;
                                for (std::int32_t at = y_begin; at < y_end; ++at) {
                                    if (at >= clip.top() && at < clip.bottom())
                                        paint(core::Point{x, at}, column, unit);
                                    acc += width;
                                    while (acc >= count) {
                                        acc -= count;
                                        ++unit;
                                    }
                                }
                            });
}

}

std::uint8_t coverage_blend(std::uint8_t base, std::uint8_t on, std::uint8_t nibble) noexcept {
    const auto lerp = [&](std::uint8_t k) { return static_cast<std::uint8_t>(((16 - k) * base + on * k) >> 4); };
    switch (nibble) {
    case 0: return base;
    case 1:
    case 3:
    case 5:
    case 6:
    case 7: return lerp(nibble);
    case 9:
    case 10:
    case 12:
    case 14: return lerp(static_cast<std::uint8_t>(nibble + 1));
    case 2: return static_cast<std::uint8_t>(base + (on >> 3) - (base >> 3));
    case 4: return static_cast<std::uint8_t>(base + (on >> 2) - (base >> 2));
    case 8: return static_cast<std::uint8_t>(base + (on >> 1) - (base >> 1));
    case 11: return static_cast<std::uint8_t>(on + (base >> 2) - (on >> 2));
    case 13: return static_cast<std::uint8_t>(on + (base >> 3) - (on >> 3));
    default: return on;
    }
}

CoverageSurface::CoverageSurface(core::Size size)
: size_{size},
  bytes_(static_cast<std::size_t>(std::max(0, size.width)) * static_cast<std::size_t>(std::max(0, size.height)), 0) {
}

void CoverageSurface::fill(std::uint8_t value) noexcept {
    std::ranges::fill(bytes_, value);
}

std::uint8_t CoverageSurface::at(core::Point p) const noexcept {
    if (!bounds().contains(p))
        return 0;
    return bytes_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)];
}

void CoverageSurface::set(core::Point p, std::uint8_t value) noexcept {
    if (!bounds().contains(p))
        return;
    bytes_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)] = value;
}

void fill_diamond_coverage(CoverageSurface& surface, core::Point anchor, CornerLifts lifts, std::uint8_t value) noexcept {
    walk_units(surface, anchor, lifts, [&](core::Point at, std::size_t, std::size_t) { surface.set(at, value); });
}

void blend_diamond_coverage(CoverageSurface& surface,
                            core::Point anchor,
                            CornerLifts lifts,
                            const formats::TransitionEntry& mask,
                            std::uint8_t on,
                            const std::uint8_t* base) noexcept {
    walk_units(surface, anchor, lifts, [&](core::Point at, std::size_t column, std::size_t unit) {
        const std::uint8_t n    = mask.coverage(column, unit);
        const std::uint8_t from = base != nullptr ? *base : surface.at(at);
        if (base == nullptr && n == 0)
            return;
        surface.set(at, coverage_blend(from, on, n));
    });
}

}
