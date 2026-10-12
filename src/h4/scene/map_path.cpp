#include "h4/scene/map_path.hpp"

#include <array>
#include <format>
#include <string_view>

namespace h4::scene {

namespace {

constexpr std::array<std::string_view, 8> kCompass{"ne", "e", "se", "s", "sw", "w", "nw", "n"};

[[nodiscard]] std::string_view colour_name(PathColour colour) noexcept {
    switch (colour) {
    case PathColour::yellow: return "yellow";
    case PathColour::red: return "red";
    case PathColour::green: break;
    }
    return "green";
}

}

std::string path_arrow_name(const PathMark& mark) {
    const std::string_view colour = colour_name(mark.colour);
    if (!mark.out || mark.in < 0 || mark.in > 7 || *mark.out < 0 || *mark.out > 7)
        return std::format("internal.{}_arrow.dest", colour);
    const int out  = *mark.out;
    const int turn = (out - mark.in) & 7;
    if (turn == 1)
        return std::format("internal.{}_arrow.right.{}", colour, kCompass[static_cast<std::size_t>(mark.in)]);
    if (turn == 7)
        return std::format("internal.{}_arrow.left.{}", colour, kCompass[static_cast<std::size_t>(mark.in)]);
    return std::format("internal.{}_arrow.straight.{}", colour, kCompass[static_cast<std::size_t>(turn == 0 ? mark.in : out)]);
}

}
