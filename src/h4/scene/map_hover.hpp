#pragma once

#include <cstdint>
#include <optional>

#include "h4/core/geometry.hpp"

namespace h4::scene {

struct MapHover {
    core::Point point;
    std::int32_t level = 0;
    std::optional<core::Point> tile;
    std::optional<std::uint32_t> sprite;
};

}
