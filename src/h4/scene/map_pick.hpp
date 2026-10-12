#pragma once

#include <cstdint>
#include <optional>

#include "h4/core/geometry.hpp"

namespace h4::formats {
class ScenarioGrid;
}

namespace h4::scene {

[[nodiscard]] std::optional<core::Point> tile_under(const formats::ScenarioGrid& grid, core::Point point, std::int32_t level);

[[nodiscard]] core::Point flat_tile_under(const formats::ScenarioGrid& grid, core::Point point) noexcept;

}
