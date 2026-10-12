#include "h4/rules/army_sight.hpp"

#include <algorithm>
#include <optional>

#include "h4/rules/army_slots.hpp"

namespace h4::rules {

namespace {

constexpr std::uint8_t kBandit = 3;

}

int army_stealth(const formats::ArmyRecord& army) {
    if (army.empty())
        return -1;
    int out = 4;
    for (const formats::ArmySlot& slot : army.slots) {
        if (slot.kind == formats::ArmySlot::Kind::hero)
            out = std::min(out, hero_rank(slot.hero, model::Discipline::stealth));
        else if (const auto kind = slot_kind(slot); kind.has_value())
            out = std::min(out, kind->value() == kBandit ? 1 : -1);
    }
    return out;
}

int army_detection(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) {
    int out = -1;
    for (const formats::ArmySlot& slot : army.slots) {
        if (slot.kind == formats::ArmySlot::Kind::hero)
            out = std::max(out, hero_rank(slot.hero, model::Discipline::scouting));
        else if (const model::CreatureType* type = slot_creature(slot, creatures))
            out = std::max(out, type->stats().level.value() - 2);
    }
    return out;
}

}
