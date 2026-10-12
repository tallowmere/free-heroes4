#pragma once

#include <cstddef>
#include <cstdint>

#include "h4/rules/adventure_step.hpp"

namespace h4::formats {
struct ArmyRecord;
}
namespace h4::model {
class CreatureCatalog;
}

namespace h4::rules {

struct ArmyMovement {
    std::int32_t day = 0;
    TerrainCosts terrain{};
};

[[nodiscard]] ArmyMovement army_movement(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures);

[[nodiscard]] std::int32_t member_movement(const formats::ArmyRecord& army, std::size_t slot, const model::CreatureCatalog& creatures);

[[nodiscard]] std::int32_t terrain_base_cost(std::uint8_t terrain) noexcept;

}
