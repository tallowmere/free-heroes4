#pragma once

#include "h4/formats/scenario/records.hpp"
#include "h4/model/creature_catalog.hpp"

namespace h4::rules {

[[nodiscard]] int army_stealth(const formats::ArmyRecord& army);

[[nodiscard]] int army_detection(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures);

[[nodiscard]] constexpr bool sees_through(int detection, int stealth) noexcept {
    return detection + 1 >= stealth;
}

}
