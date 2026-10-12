#include "h4/graphics/terrain_blit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <span>

#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/shade565.hpp"

namespace h4::graphics {

namespace {

[[nodiscard]] core::Rect column_window(const core::Rect& clip) noexcept {
    constexpr std::int32_t far = std::numeric_limits<std::int32_t>::max() / 4;
    return core::Rect::from_edges(clip.left(), -far, clip.right(), far);
}

struct SlopeWalk {
    std::int32_t start = kFlatSlope;
    std::int32_t end   = kFlatSlope;
};

[[nodiscard]] SlopeWalk slope_at(CornerSlopes s, std::int32_t column) noexcept {
    if (column < 32)
        return {s.left + column * ((s.top - s.left) / 32), s.left + column * ((s.bottom - s.left) / 32)};
    const std::int32_t k = column - 32;
    return {s.top + k * ((s.right - s.top) / 32), s.bottom + k * ((s.right - s.bottom) / 32)};
}

class ColumnShade {
public:
    ColumnShade(CornerSlopes slopes, std::int32_t column, std::int32_t count) noexcept : parity_{column & 1} {
        const SlopeWalk walk = slope_at(slopes, column);
        coord_               = walk.start;
        step_                = (walk.end - walk.start) / count;
    }

    [[nodiscard]] unsigned index(std::int32_t row_parity) const noexcept {
        return static_cast<unsigned>(shade_index(coord_ + 0x100, row_parity, parity_));
    }
    void advance(std::int32_t pixels = 1) noexcept {
        coord_ += pixels * step_;
    }

private:
    std::int32_t coord_ = 0;
    std::int32_t step_  = 0;
    std::int32_t parity_;
};

template <class Paint>
void walk_tile(const Canvas& canvas,
               const formats::TerrainTexture& texture,
               std::size_t tile,
               core::Point anchor,
               CornerLifts lifts,
               CornerSlopes slopes,
               std::int32_t parity_origin,
               Paint paint) {
    const std::span<const std::uint8_t> widths = formats::terrain_row_widths();

    for_each_diamond_column(anchor, lifts, column_window(canvas.local_clip()), [&](std::int32_t x, std::int32_t y_begin, std::int32_t y_end) {
        const std::int32_t column = x - anchor.x;
        const std::int32_t count  = y_end - y_begin;
        if (count <= 0)
            return;

        const std::size_t source                = static_cast<std::size_t>(column);
        const std::span<const std::uint8_t> row = texture.row(tile, source);
        const std::int32_t width                = widths[source];
        if (row.empty() || width <= 0)
            return;

        ColumnShade shading{slopes, column, count};

        std::int32_t counter = count;
        std::int32_t at      = y_begin;
        std::size_t cursor   = 0;
        std::size_t unit     = 0;
        bool skipping        = true;

        while (cursor < row.size() && at < y_end) {
            const std::uint8_t run = row[cursor++];
            if (skipping) {
                counter += run * count;
                if (counter > count) {
                    const std::int32_t rows = (counter - count + width - 1) / width;
                    counter -= rows * width;
                    at += rows;
                    shading.advance(rows);
                }
                unit += run;
            } else {
                for (std::uint8_t i = 0; i < run && at < y_end; ++i, ++unit) {
                    const formats::PaletteColor colour = texture.colour(row[cursor + i]);
                    const Color565 pixel               = Color565::from_rgb(colour.r, colour.g, colour.b);
                    while (counter > 0 && at < y_end) {
                        paint(core::Point{x, at}, shade(pixel, shading.index((at - parity_origin) & 1)), source, unit);
                        ++at;
                        shading.advance();
                        counter -= width;
                    }
                    counter += count;
                }
                cursor += run;
            }
            skipping = !skipping;
        }
    });
}

}

std::uint16_t slope_coordinate(std::int32_t upper, std::int32_t lower) noexcept {
    constexpr double kRowSpacing = 64.0 / std::numbers::sqrt2;
    constexpr double kFloor      = 0x1555;
    constexpr double kSpan       = 54614.0;

    double angle = std::atan2(static_cast<double>(upper - lower), kRowSpacing) + 3.0 * std::numbers::pi / 4.0;
    if (angle > std::numbers::pi)
        angle = 2.0 * std::numbers::pi - angle;
    const double rise = angle - std::numbers::pi / 2.0;
    if (rise <= 0.0)
        return static_cast<std::uint16_t>(kFloor);
    return static_cast<std::uint16_t>(kFloor + static_cast<std::int32_t>(rise * kSpan / (std::numbers::pi / 2.0)));
}

void fill_terrain_tile(const Canvas& canvas, core::Point anchor, CornerLifts lifts, Color565 colour) {
    for_each_diamond_column(anchor, lifts, canvas.local_clip(), [&](std::int32_t x, std::int32_t y_begin, std::int32_t y_end) {
        for (std::int32_t y = y_begin; y < y_end; ++y)
            canvas.set_pixel({x, y}, colour);
    });
}

void blit_terrain_tile(const Canvas& canvas, const formats::TerrainTexture& texture, std::size_t tile, core::Point anchor, CornerLifts lifts, CornerSlopes slopes) {
    walk_tile(canvas, texture, tile, anchor, lifts, slopes, 0, [&](core::Point at, Color565 pixel, std::size_t, std::size_t) {
        canvas.set_pixel(at, pixel);
    });
}

TileStencil::TileStencil() noexcept {
    for (std::size_t c = 0; c < bits_.size(); ++c) {
        const std::size_t units = formats::transition_column_units(c);
        bits_[c] = units >= 32 ? ~std::uint32_t{0} : static_cast<std::uint32_t>((std::uint32_t{1} << units) - 1);
    }
}

void TileStencil::clear(const formats::TransitionEntry& entry) noexcept {
    for (std::size_t c = 0; c < bits_.size(); ++c)
        for (std::size_t u = 0; u < formats::transition_column_units(c); ++u)
            if (entry.coverage(c, u) != 0)
                bits_[c] &= ~(std::uint32_t{1} << u);
}

bool TileStencil::at(std::size_t column, std::size_t unit) const noexcept {
    if (column >= bits_.size() || unit >= 32)
        return false;
    return (bits_[column] >> unit & 1U) != 0;
}

std::uint8_t TileCoverage::at(std::size_t column, std::size_t unit) const noexcept {
    if (stencil != nullptr && !stencil->at(column, unit))
        return 0;
    return entry == nullptr ? std::uint8_t{15} : entry->coverage(column, unit);
}

void blit_transition_tile(const Canvas& canvas,
                          const formats::TerrainTexture& texture,
                          std::size_t tile,
                          core::Point anchor,
                          CornerLifts lifts,
                          CornerSlopes slopes,
                          TileCoverage coverage) {
    walk_tile(canvas, texture, tile, anchor, lifts, slopes, anchor.y, [&](core::Point at, Color565 pixel, std::size_t column, std::size_t unit) {
        if (const std::uint8_t n = coverage.at(column, unit); n != 0)
            canvas.blend_pixel(at, pixel, coverage_weight(n));
    });
}

TileOffscreen::TileOffscreen() noexcept {
    const std::span<const std::uint8_t> widths = formats::terrain_row_widths();
    for (std::size_t r = 0; r < kRows; ++r)
        starts_[r + 1] = static_cast<std::uint16_t>(starts_[r] + (r < widths.size() ? widths[r] : 0));
}

void TileOffscreen::render(const formats::TerrainTexture& texture, std::size_t tile, std::size_t first, std::size_t last) noexcept {
    const std::span<const std::uint8_t> widths = formats::terrain_row_widths();
    last                                       = std::min(last, widths.size());
    std::size_t start                          = starts_[std::min(first, kRows)];
    for (std::size_t r = first; r < last; start += widths[r], ++r) {
        const std::span<const std::uint8_t> row = texture.row(tile, r);
        std::size_t unit   = 0;
        std::size_t cursor = 0;
        bool skipping      = true;
        while (cursor < row.size() && unit < widths[r]) {
            const std::size_t run = std::min<std::size_t>(row[cursor++], widths[r] - unit);
            if (!skipping) {
                for (std::size_t i = 0; i < run && cursor + i < row.size(); ++i) {
                    const formats::PaletteColor colour = texture.colour(row[cursor + i]);
                    units_[start + unit + i]           = Color565::from_rgb(colour.r, colour.g, colour.b);
                }
                cursor += run;
            }
            unit += run;
            skipping = !skipping;
        }
    }
}

Color565 TileOffscreen::at(std::size_t row, std::size_t unit) const noexcept {
    const std::size_t index = starts_[std::min(row, kRows)] + unit;
    return index < units_.size() ? units_[index] : Color565{};
}

void composite_transition_tile(const Canvas& canvas,
                               const TileOffscreen& offscreen,
                               core::Point anchor,
                               CornerLifts lifts,
                               CornerSlopes slopes,
                               const formats::TransitionEntry& entry) {
    const std::span<const std::uint8_t> widths = formats::terrain_row_widths();

    for_each_diamond_column(anchor, lifts, column_window(canvas.local_clip()), [&](std::int32_t x, std::int32_t y_begin, std::int32_t y_end) {
        const std::int32_t column = x - anchor.x;
        const std::int32_t count  = y_end - y_begin;
        if (count <= 0)
            return;
        const std::size_t row    = static_cast<std::size_t>(column);
        const std::int32_t width = widths[row];

        ColumnShade shading{slopes, column, count};

        std::int32_t spent = 0;
        std::size_t unit   = 0;
        for (std::int32_t at = y_begin; at < y_end; ++at, shading.advance()) {
            if (const std::uint8_t n = entry.coverage(row, unit); n != 0)
                canvas.blend_pixel({x, at}, shade(offscreen.at(row, unit), shading.index((at - anchor.y) & 1)), coverage_weight(n));
            for (spent += width; spent >= count; spent -= count)
                ++unit;
        }
    });
}

}
