#include "h4/scene/map_pick.hpp"

#include "h4/formats/scenario/grid.hpp"
#include "h4/graphics/diamond.hpp"
#include "h4/scene/map_edges.hpp"
#include "h4/scene/map_lattice.hpp"

namespace h4::scene {

namespace {

[[nodiscard]] std::int32_t along(std::int32_t from, std::int32_t to, std::int32_t t) noexcept {
    return graphics::half_column_lerp(from, to, t);
}

[[nodiscard]] std::int32_t top_edge(const formats::ScenarioGrid& grid, std::int32_t x, std::int32_t level) {
    const std::int32_t c     = x / kHalfTile;
    const std::int32_t f     = x % kHalfTile;
    const std::int32_t left  = top_edge_lift(grid, c, level);
    const std::int32_t right = top_edge_lift(grid, c + 1, level);
    return left <= right ? along(left, right, f) : along(right, left, 31 - f);
}

[[nodiscard]] std::int32_t bottom_edge(const formats::ScenarioGrid& grid, std::int32_t x, std::int32_t level) {
    const std::int32_t c     = x / kHalfTile;
    const std::int32_t f     = x % kHalfTile;
    const std::int32_t left  = bottom_edge_lift(grid, c, level);
    const std::int32_t right = bottom_edge_lift(grid, c + 1, level);
    return left >= right ? along(left, right, f) : along(right, left, 31 - f);
}

}

core::Point flat_tile_under(const formats::ScenarioGrid& grid, core::Point point) noexcept {
    const std::int32_t width = grid.width();
    const std::int32_t a     = 2 * point.x - width * kTileWidth / 2;
    const std::int32_t b     = (2 * point.y - width * kTileHeight / 2) * 2;
    const std::int32_t c     = width * 64;
    return {(c - a + b + 2) / 128, (c + a + b + 1) / 128};
}

std::optional<core::Point> tile_under(const formats::ScenarioGrid& grid, core::Point point, std::int32_t level) {
    const std::int32_t width = grid.width();
    const std::int32_t box_w = width * kTileWidth / 2;
    const std::int32_t box_h = width * kTileHeight / 2;
    const std::int32_t x     = point.x;
    const std::int32_t y     = point.y;
    if (x < 0 || y < -kRaiseAllowance || x >= box_w || y >= box_h)
        return std::nullopt;
    if (y < 0 && y < -top_edge(grid, x, level))
        return std::nullopt;
    if (y >= box_h - kRaiseAllowance && y >= box_h - bottom_edge(grid, x, level))
        return std::nullopt;

    core::Point tile = flat_tile_under(grid, {x, y < 0 ? 0 : y});
    const core::Point pin  = box_pin(tile, width);
    std::int32_t px        = pin.x;
    std::int32_t py        = pin.y;
    const bool right       = x >= px;
    std::int32_t t         = right ? px - x + 31 : x - px + 32;
    const core::Point side = right ? core::Point{tile.x, tile.y + 1} : core::Point{tile.x + 1, tile.y};
    std::int32_t side_y    = py + 16 - grid.elevation(side.x, side.y, level);

    for (std::int32_t step = 0; step <= 2 * width + kRaiseAllowance / 16; ++step) {
        const std::int32_t bottom_y = py + 32 - grid.elevation(tile.x + 1, tile.y + 1, level);
        if (along(side_y, bottom_y, t) > y)
            return tile;
        t      = 31 - t;
        side_y = bottom_y;
        if (x >= px) {
            px += kHalfTile;
            ++tile.y;
        } else {
            px -= kHalfTile;
            ++tile.x;
        }
        py += 16;
    }
    return std::nullopt;
}

}
