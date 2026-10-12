#include "h4/cursor/direction.hpp"

#include <array>
#include <utility>

namespace h4::cursor {

namespace {

constexpr std::array<std::string_view, 8> kNames8{"north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"};

constexpr std::array<std::string_view, 8> kShortNames8{"n", "ne", "e", "se", "s", "sw", "w", "nw"};

constexpr std::array<std::string_view, 16> kNames16{
    "northeast",
    "east_northeast",
    "east",
    "east_southeast",
    "southeast",
    "south_southeast",
    "south",
    "south_southwest",
    "southwest",
    "west_southwest",
    "west",
    "west_northwest",
    "northwest",
    "north_northwest",
    "north",
    "north_northeast",
};

template <class Names, class Enum> std::string_view lookup(const Names& names, Enum d) noexcept {
    const auto i = std::to_underlying(d);
    return i < names.size() ? names[i] : std::string_view{};
}

}

std::string_view to_string(Direction8 d) noexcept {
    return lookup(kNames8, d);
}

std::string_view to_string(Direction16 d) noexcept {
    return lookup(kNames16, d);
}

std::string_view short_name(Direction8 d) noexcept {
    return lookup(kShortNames8, d);
}

}
