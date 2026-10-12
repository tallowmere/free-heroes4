#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "h4/core/geometry.hpp"

namespace h4::scene {

enum class PathColour : std::uint8_t { green, yellow, red };

struct PathMark {
    core::Point tile;
    std::int32_t level = 0;
    int in             = 0;
    std::optional<int> out;
    PathColour colour = PathColour::green;
    bool bridge       = false;
};

[[nodiscard]] std::string path_arrow_name(const PathMark& mark);

}
