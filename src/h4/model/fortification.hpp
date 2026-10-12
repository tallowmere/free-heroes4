#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

#include "h4/formats/scenario/objects.hpp"

namespace h4::model {

enum class Fortification : std::uint8_t { village, fort, citadel, castle };

[[nodiscard]] constexpr Fortification fortification_of(const formats::TownBuildings& buildings) noexcept {
    if (buildings.has(5))
        return Fortification::castle;
    if (buildings.has(4))
        return Fortification::citadel;
    if (buildings.has(3))
        return Fortification::fort;
    return Fortification::village;
}

[[nodiscard]] constexpr std::string_view to_string(Fortification tier) noexcept {
    constexpr std::array<std::string_view, 4> kWords{"village", "fort", "citadel", "castle"};
    return kWords[std::to_underlying(tier)];
}

}
