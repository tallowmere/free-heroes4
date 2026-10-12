#pragma once

#include <cstdint>
#include <string_view>

namespace h4::cursor {

enum class Direction8 : std::uint8_t { north, northeast, east, southeast, south, southwest, west, northwest };

enum class Direction16 : std::uint8_t {
    northeast,
    east_northeast,
    east,
    east_southeast,
    southeast,
    south_southeast,
    south,
    south_southwest,
    southwest,
    west_southwest,
    west,
    west_northwest,
    northwest,
    north_northwest,
    north,
    north_northeast
};

[[nodiscard]] std::string_view to_string(Direction8 d) noexcept;
[[nodiscard]] std::string_view to_string(Direction16 d) noexcept;

[[nodiscard]] std::string_view short_name(Direction8 d) noexcept;

}
