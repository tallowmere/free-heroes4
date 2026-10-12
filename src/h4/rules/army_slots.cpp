#include "h4/rules/army_slots.hpp"

#include <algorithm>

#include "h4/model/creature_catalog.hpp"
#include "h4/model/skills.hpp"

namespace h4::rules {

std::optional<model::CreatureKindId> slot_kind(const formats::ArmySlot& slot) noexcept {
    const auto id = slot.creature_kind_id();
    return id.has_value() ? std::optional{model::CreatureKindId{*id}} : std::nullopt;
}

const model::CreatureType* slot_creature(const formats::ArmySlot& slot, const model::CreatureCatalog& creatures) noexcept {
    const auto kind = slot_kind(slot);
    return kind.has_value() ? creatures.find(*kind) : nullptr;
}

int hero_rank(const formats::HeroRecord& hero, model::Discipline discipline) {
    for (const model::HeroSkill& skill : model::hero_skills(hero))
        if (skill.discipline == discipline)
            return skill.rank;
    return -1;
}

std::int32_t own_movement_points(std::int32_t move) noexcept {
    const std::int64_t points = 11 * std::int64_t{move} * 100 / 25 + 1200;
    return static_cast<std::int32_t>(std::min(points, std::int64_t{3000}));
}

std::int32_t fastest_creature_points(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) noexcept {
    std::int32_t fastest = 0;
    for (const formats::ArmySlot& slot : army.slots)
        if (const model::CreatureType* type = slot_creature(slot, creatures))
            fastest = std::max(fastest, own_movement_points(type->stats().movement));
    return fastest;
}

std::optional<std::int32_t>
member_own_points(const formats::ArmySlot& slot, std::int32_t fastest, const model::CreatureCatalog& creatures) noexcept {
    if (slot.kind == formats::ArmySlot::Kind::hero)
        return std::max(own_movement_points(kHeroMove), fastest);
    if (const model::CreatureType* type = slot_creature(slot, creatures))
        return own_movement_points(type->stats().movement);
    return std::nullopt;
}

}
