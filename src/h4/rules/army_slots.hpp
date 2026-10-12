#pragma once

#include <cstdint>
#include <optional>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/creature.hpp"
#include "h4/model/skills.hpp"

namespace h4::model {
class CreatureCatalog;
}

namespace h4::rules {

[[nodiscard]] std::optional<model::CreatureKindId> slot_kind(const formats::ArmySlot& slot) noexcept;
[[nodiscard]] const model::CreatureType* slot_creature(const formats::ArmySlot& slot, const model::CreatureCatalog& creatures) noexcept;

inline constexpr std::int32_t kHeroMove = 24;

[[nodiscard]] int hero_rank(const formats::HeroRecord& hero, model::Discipline discipline);

[[nodiscard]] std::int32_t own_movement_points(std::int32_t move) noexcept;

[[nodiscard]] std::int32_t fastest_creature_points(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) noexcept;

[[nodiscard]] std::optional<std::int32_t>
member_own_points(const formats::ArmySlot& slot, std::int32_t fastest, const model::CreatureCatalog& creatures) noexcept;

}
