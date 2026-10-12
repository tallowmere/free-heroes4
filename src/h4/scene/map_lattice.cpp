#include "h4/scene/map_lattice.hpp"

#include "h4/formats/scenario/grid.hpp"

namespace h4::scene {

namespace {

struct Wide {
    std::int64_t x;
    std::int64_t y;
};

[[nodiscard]] Wide wide_anchor(std::int32_t x, std::int32_t y, std::int32_t width) noexcept {
    return {32 * (std::int64_t{y} - x) + 32 * (std::int64_t{width} - 1), 16 * (std::int64_t{x} + y)};
}

[[nodiscard]] Wide box_origin(std::int32_t width) noexcept {
    return {std::int64_t{width} * kTileWidth / 4, std::int64_t{width} * kTileHeight / 4};
}

[[nodiscard]] core::Point narrowed(Wide at) noexcept {
    return {static_cast<std::int32_t>(at.x), static_cast<std::int32_t>(at.y)};
}

}

core::Point tile_anchor(std::int32_t x, std::int32_t y, std::int32_t width) noexcept {
    return narrowed(wide_anchor(x, y, width));
}

core::Point tile_pin(std::int32_t x, std::int32_t y, std::int32_t width) noexcept {
    return tile_anchor(x, y, width) + core::Point{kTileWidth / 2, 0};
}

core::Rect map_box(const formats::ScenarioGrid& grid) noexcept {
    const std::int32_t width = grid.width();
    return core::Rect{narrowed(box_origin(width)), {width * kTileWidth / 2, width * kTileHeight / 2}};
}

core::Point box_anchor(std::int32_t x, std::int32_t y, std::int32_t width) noexcept {
    const Wide at = wide_anchor(x, y, width), origin = box_origin(width);
    return narrowed({at.x - origin.x, at.y - origin.y});
}

core::Point box_pin(core::Point cell, std::int32_t width, std::int32_t height) noexcept {
    const Wide at = wide_anchor(cell.x, cell.y, width), origin = box_origin(width);
    return narrowed({at.x - origin.x + kTileWidth / 2, at.y - origin.y - height});
}

}
