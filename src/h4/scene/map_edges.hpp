#pragma once

#include <cstdint>

#include "h4/core/geometry.hpp"
#include "h4/graphics/tiled_fill.hpp"

namespace h4::formats {
class ScenarioGrid;
}
namespace h4::graphics {
class Canvas;
class Surface565;
}

namespace h4::scene {

[[nodiscard]] std::int32_t top_edge_lift(const formats::ScenarioGrid& grid, std::int32_t boundary, std::int32_t level) noexcept;
[[nodiscard]] std::int32_t bottom_edge_lift(const formats::ScenarioGrid& grid, std::int32_t boundary, std::int32_t level) noexcept;

struct EdgeBand {
    core::Rect flat;
    core::Rect wedge;
    graphics::WedgeCorner corner;
};
[[nodiscard]] EdgeBand top_edge_band(const formats::ScenarioGrid& grid, std::int32_t column, std::int32_t level) noexcept;
[[nodiscard]] EdgeBand bottom_edge_band(const formats::ScenarioGrid& grid, std::int32_t column, std::int32_t level) noexcept;

void paint_map_edges(const graphics::Canvas& canvas,
                     const graphics::Surface565& texture,
                     const formats::ScenarioGrid& grid,
                     std::int32_t level,
                     core::Point camera);

}
