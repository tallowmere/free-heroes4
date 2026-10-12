#include "h4/model/starting_troops.hpp"

#include <optional>
#include <span>
#include <utility>

#include "h4/model/army_member.hpp"
#include "h4/model/creature_catalog.hpp"
#include "h4/model/map.hpp"
#include "h4/model/random_objects.hpp"
#include "h4/model/random_passes.hpp"

namespace h4::model {

namespace {

constexpr std::array<std::array<StartingStack, 2>, 6> kStacks{{
    {{{"squire", false}, {"crossbowman", true}}},
    {{{"dwarf", false}, {"halfling", true}}},
    {{{"skeleton", false}, {"imp", false}}},
    {{{"bandit", false}, {"orc", true}}},
    {{{"wolf", false}, {"sprite", false}}},
    {{{"berserker", false}, {"centaur", true}}},
}};

constexpr std::array<std::size_t, 7> kShooterOrder{3, 1, 5, 2, 4, 0, 6};
constexpr std::array<std::size_t, 7> kFighterOrder{2, 4, 0, 6, 3, 1, 5};

const formats::ArmySlot* leading_member(const formats::ArmyRecord& army, const CreatureCatalog& creatures) {
    const formats::ArmySlot* best = nullptr;
    const auto experience         = [&](const formats::ArmySlot& slot) {
        const auto id            = slot.creature_kind_id();
        const CreatureType* type = id ? creatures.find(CreatureKindId{*id}) : nullptr;
        return type == nullptr ? 0 : type->stats().experience.value();
    };
    for (const formats::ArmySlot& slot : army.slots) {
        if (slot.vacant())
            continue;
        if (best == nullptr) {
            best = &slot;
            continue;
        }
        const bool hero      = slot.kind == formats::ArmySlot::Kind::hero;
        const bool best_hero = best->kind == formats::ArmySlot::Kind::hero;
        if (hero != best_hero) {
            if (hero)
                best = &slot;
            continue;
        }
        if (hero ? slot.hero.level > best->hero.level : experience(slot) > experience(*best))
            best = &slot;
    }
    return best;
}

std::optional<Alignment> army_alignment(const MapActor& actor, const RandomResolution& choices, const CreatureCatalog& creatures) {
    if (actor.owned())
        return owner_alignment(actor, choices);
    if (actor.army.empty())
        return actor.alignment;
    const formats::ArmySlot* member = leading_member(actor.army, creatures);
    return member == nullptr ? std::nullopt : member_alignment(*member, creatures);
}

void add_stack(formats::ArmyRecord& army, const CreatureType& type, bool ranged) {
    const std::int16_t kind = type.id().value();
    for (const std::size_t index : ranged ? kShooterOrder : kFighterOrder) {
        formats::ArmySlot& slot = army.slots[index];
        if (slot.vacant()) {
            slot.kind  = formats::ArmySlot::Kind::creature;
            slot.stack = {kind, static_cast<std::int16_t>(type.stats().weekly_growth), {}};
            return;
        }
        if (slot.kind == formats::ArmySlot::Kind::creature && slot.stack.creature == kind) {
            slot.stack.count = static_cast<std::int16_t>(slot.stack.count + type.stats().weekly_growth);
            return;
        }
    }
}

}

const std::array<StartingStack, 2>& starting_stacks(Alignment alignment) noexcept {
    return kStacks[std::to_underlying(alignment)];
}

void Map::add_starting_troops(const RandomResolution& choices) {
    if (choices.creatures == nullptr)
        return;
    for (MapActor& actor : actors_) {
        if (!actor.starting_troops)
            continue;
        const std::optional<Alignment> alignment = army_alignment(actor, choices, *choices.creatures);
        if (!alignment.has_value())
            continue;
        for (const StartingStack& stack : starting_stacks(*alignment))
            if (const CreatureType* type = choices.creatures->find(stack.creature); type != nullptr)
                add_stack(actor.army, *type, stack.ranged);
    }
}

}
