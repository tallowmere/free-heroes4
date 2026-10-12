#include "h4/scene/map_edges.hpp"

#include <algorithm>

#include "h4/formats/scenario/grid.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/scene/map_lattice.hpp"

namespace h4::scene {

namespace {

void paint_band(const graphics::Canvas& canvas, const graphics::Surface565& texture, const EdgeBand& band, core::Point camera) {
    const core::Point view = core::Point{} - camera;
    graphics::fill_tiled(canvas, texture, band.flat.translated(view), band.flat.origin());
    if (band.wedge.empty())
        return;
    graphics::fill_wedge(canvas, texture, band.wedge.translated(view), band.wedge.origin(), band.corner);
}

}

std::int32_t top_edge_lift(const formats::ScenarioGrid& grid, std::int32_t boundary, std::int32_t level) noexcept {
    const std::int32_t half = grid.width() / 2;
    const std::int32_t k    = boundary / 2;
    return boundary % 2 == 0 ? grid.elevation(half - k, k, level) : grid.elevation(half - 1 - k, k, level);
}

std::int32_t bottom_edge_lift(const formats::ScenarioGrid& grid, std::int32_t boundary, std::int32_t level) noexcept {
    const std::int32_t width = grid.width();
    const std::int32_t k     = boundary / 2;
    return boundary % 2 == 0 ? grid.elevation(width - k, width / 2 + k, level) :
                               grid.elevation(width - k, width / 2 + k + 1, level);
}

EdgeBand top_edge_band(const formats::ScenarioGrid& grid, std::int32_t column, std::int32_t level) noexcept {
    const std::int32_t left  = top_edge_lift(grid, column, level);
    const std::int32_t right = top_edge_lift(grid, column + 1, level);
    const std::int32_t high  = std::max(left, right);
    const std::int32_t low   = std::min(left, right);
    const std::int32_t x     = column * kHalfTile;
    return {core::Rect::from_edges(x, -kRaiseAllowance, x + kHalfTile, -high),
            core::Rect::from_edges(x, -high, x + kHalfTile, -low),
            right < left ? graphics::WedgeCorner::top_right : graphics::WedgeCorner::top_left};
}

EdgeBand bottom_edge_band(const formats::ScenarioGrid& grid, std::int32_t column, std::int32_t level) noexcept {
    const std::int32_t left   = bottom_edge_lift(grid, column, level);
    const std::int32_t right  = bottom_edge_lift(grid, column + 1, level);
    const std::int32_t high   = std::max(left, right);
    const std::int32_t low    = std::min(left, right);
    const std::int32_t x      = column * kHalfTile;
    const std::int32_t bottom = map_box(grid).height();
    return {core::Rect::from_edges(x, bottom - low, x + kHalfTile, bottom),
            core::Rect::from_edges(x, bottom - high, x + kHalfTile, bottom - low),
            left < right ? graphics::WedgeCorner::bottom_right : graphics::WedgeCorner::bottom_left};
}

void paint_map_edges(const graphics::Canvas& canvas,
                     const graphics::Surface565& texture,
                     const formats::ScenarioGrid& grid,
                     std::int32_t level,
                     core::Point camera) {
    const core::Rect clip = canvas.local_clip();
    if (clip.empty() || texture.empty())
        return;
    const std::int32_t width  = grid.width();
    const std::int32_t bottom = map_box(grid).height();
    const auto [first, last]  = edge_columns(clip.left() + camera.x, clip.right() + camera.x, width);

    if (clip.top() + camera.y < 0)
        for (std::int32_t c = first; c < last; ++c)
            paint_band(canvas, texture, top_edge_band(grid, c, level), camera);
    if (bottom - kRaiseAllowance < clip.bottom() + camera.y)
        for (std::int32_t c = first; c < last; ++c)
            paint_band(canvas, texture, bottom_edge_band(grid, c, level), camera);
}

}
