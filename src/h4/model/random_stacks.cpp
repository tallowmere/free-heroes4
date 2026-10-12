#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "h4/model/creature_catalog.hpp"
#include "h4/model/map.hpp"
#include "h4/model/random_objects.hpp"
#include "h4/model/random_passes.hpp"

namespace h4::model {

namespace {

[[nodiscard]] bool in(std::span<const int> set, int kind) noexcept {
    return std::ranges::find(set, kind) != set.end();
}

constexpr std::array<std::int32_t, 4> kTierBase{310, 775, 1860, 4960};
constexpr std::array<std::int32_t, 5> kDifficultyNumerator{2, 1, 3, 2, 3};
constexpr std::array<std::int32_t, 5> kDifficultyDenominator{3, 1, 2, 1, 1};
constexpr std::int32_t kPeasantExperience = 4;
constexpr std::array<int, 3> kSeaKinds{38, 50, 52};
constexpr std::array<int, 2> kShoalKinds{38, 52};

template <class Admits>
[[nodiscard]] const CreatureType* roll_filtered(const CreatureCatalog& creatures, Roller& roller, Admits admits) {
    const CreatureType* kept = nullptr;
    std::int32_t seen        = 0;
    for (const CreatureType& type : creatures.all()) {
        if (!admits(type))
            continue;
        ++seen;
        if (roller.next(seen) == 0)
            kept = &type;
    }
    return kept;
}

[[nodiscard]] const CreatureType* roll_kind(const CreatureCatalog& creatures,
                                            Roller& roller,
                                            std::optional<Alignment> filter,
                                            bool on_water,
                                            std::int32_t cap,
                                            int level_low,
                                            int level_high,
                                            int anchor) {
    return roll_filtered(creatures, roller, [&](const CreatureType& type) {
        const int kind                = type.id().value();
        const std::int32_t experience = type.stats().experience.value();
        if (experience <= 0 || experience > cap)
            return false;
        if (experience == cap && kind != anchor)
            return false;
        const int level = type.stats().level.value();
        if (level < level_low || level > level_high)
            return false;
        if (filter.has_value() && type.alignment() != *filter)
            return false;
        return in(kSeaKinds, kind) == on_water;
    });
}

[[nodiscard]] std::int32_t stack_size(int kind, std::int32_t experience, std::int32_t strength) noexcept {
    if (experience <= 0)
        return 1;
    const std::int64_t count = in(kShoalKinds, kind) ?
        std::int64_t{strength} / experience :
        (3 * std::int64_t{experience} - 1 + 2 * std::int64_t{strength}) / (3 * std::int64_t{experience});
    return static_cast<std::int32_t>(std::max(count, std::int64_t{1}));
}

constexpr std::array<std::string_view, 74> kDwellings{"Altar of Air",
                                                      "Altar of Light",
                                                      "Ballista Works",
                                                      "Den of Thieves",
                                                      "Behemoth Crag",
                                                      "PillarOfEyes",
                                                      "Dragon Cave",
                                                      "Dragon Graveyard",
                                                      "Centaur Stables",
                                                      "Kennels",
                                                      "Knight's Chapter",
                                                      "Archery Range",
                                                      "Barracks",
                                                      "Cyclops Cave",
                                                      "Spawning Pit",
                                                      "Temple of the Damned",
                                                      "Dragon Factory",
                                                      "Dwarven Mines",
                                                      "Altar of Earth",
                                                      "Lava Tube",
                                                      "Homestead",
                                                      "Magic Forest",
                                                      "Altar of Fire",
                                                      "Parapet",
                                                      "Altar of Wishes",
                                                      "Barrow Mound",
                                                      "Berserker Dwelling",
                                                      "Golem Factory",
                                                      "Griffin Cliffs",
                                                      "Halfling Burrow",
                                                      "Harpy Nest",
                                                      "Hydra Pond",
                                                      "Ice Cave",
                                                      "Torture Chamber",
                                                      "Faerie Mound",
                                                      "Mage Tower",
                                                      "Mantis Nest",
                                                      "Statuary Garden",
                                                      "",
                                                      "Minotaur's Maze",
                                                      "Monestary",
                                                      "Cursed Pyramid",
                                                      "Golden Pavilion",
                                                      "Black Wood",
                                                      "Nomad Tents",
                                                      "Ogre Fort",
                                                      "Orc Camp",
                                                      "Hovel",
                                                      "Funeral Pyre",
                                                      "Guardhouse",
                                                      "Pirates Cove",
                                                      "Enchanted Grove",
                                                      "",
                                                      "Cemetery",
                                                      "Squire's Guild",
                                                      "Fae Trees",
                                                      "Cliff Nest",
                                                      "Cloud Castle",
                                                      "Troglodyte Warren",
                                                      "Troll Cave",
                                                      "Unicorn Glade",
                                                      "Mansion",
                                                      "Altar of Water",
                                                      "Tiger Den",
                                                      "Wolf Den",
                                                      "Crypt",
                                                      "hot_house",
                                                      "Goblin_Armory",
                                                      "Ward_Sorcery",
                                                      "Gargantuan_Dell",
                                                      "Dark_Knights_Sanctum",
                                                      "Siege_Workshop",
                                                      "Beast_Pen",
                                                      ""};

constexpr int kPirate = 50;

}

void resolve_random_monster(MapActor& actor, const RandomResolution& choices, Roller& roller, bool on_water) {
    if (choices.creatures == nullptr)
        return;
    const bool any  = actor.family == "random_random_monster";
    const int level = any ? 3 : sub_level(actor.sub);
    if (level < 1 || level > static_cast<int>(kTierBase.size()))
        return;
    const int level_low  = any ? 0 : level;
    const int level_high = any ? 5 : level;

    const std::int64_t low = actor.band_high >= 1 ? std::int64_t{actor.band_low} * kPeasantExperience :
                                                    std::int64_t{kTierBase[static_cast<std::size_t>(level - 1)]} * 4 / 5;
    std::int64_t high = actor.band_high >= 1 ? std::int64_t{actor.band_high} * kPeasantExperience :
                                               std::int64_t{kTierBase[static_cast<std::size_t>(level - 1)]} * 6 / 5;
    if (high < low)
        high = low;

    const auto grade            = std::to_underlying(choices.difficulty);
    const std::int64_t span     = std::min(high - low + 1, std::int64_t{std::numeric_limits<std::int32_t>::max()});
    const std::int64_t rolled   = low + roller.next(static_cast<std::int32_t>(span));
    const std::int64_t strength = grade < kDifficultyNumerator.size() ?
        rolled * kDifficultyNumerator[grade] / kDifficultyDenominator[grade] :
        rolled;

    const CreatureType* kind = roll_kind(
        *choices.creatures, roller, std::nullopt, on_water, static_cast<std::int32_t>(strength), level_low, level_high, -1
    );
    if (kind == nullptr)
        return;

    const int id                  = kind->id().value();
    const std::int32_t experience = kind->stats().experience.value();
    const std::int32_t count      = stack_size(id, experience, static_cast<std::int32_t>(strength));

    formats::ArmyRecord army;
    army.slots[0].kind           = formats::ArmySlot::Kind::creature;
    army.slots[0].stack.creature = static_cast<std::int16_t>(id);
    army.slots[0].stack.count    = static_cast<std::int16_t>(std::min(count, 32767));

    if (const std::int64_t left = strength - std::int64_t{experience} * count; left > 0) {
        const int mate_level     = kind->stats().level.value();
        const CreatureType* mate = roll_kind(*choices.creatures,
                                             roller,
                                             kind->alignment(),
                                             on_water,
                                             static_cast<std::int32_t>(std::min(left, std::int64_t{experience})),
                                             std::max(mate_level - 1, 0),
                                             mate_level,
                                             id);
        if (mate != nullptr)
            if (const std::int64_t mates = left / mate->stats().experience.value(); mates > 0) {
                army.slots[1].kind           = formats::ArmySlot::Kind::creature;
                army.slots[1].stack.creature = mate->id().value();
                army.slots[1].stack.count    = static_cast<std::int16_t>(std::min(mates, std::int64_t{32767}));
            }
    }

    actor.army  = army;
    actor.kind  = ActorKind::monster;
    actor.owner = formats::kNoOwner;
    actor.model.clear();
    actor.family.clear();
    actor.sub.clear();
}

void resolve_random_dwelling(MapActor& actor, const RandomResolution& choices, Roller& roller) {
    if (choices.creatures == nullptr)
        return;
    const int level = sub_level(actor.sub);
    if (level < 1)
        return;

    const std::optional<Alignment> filter = alignment_constraint(actor, choices);
    const CreatureType* kept              = roll_filtered(*choices.creatures, roller, [&](const CreatureType& type) {
        const int kind = type.id().value();
        if (kind == kPirate || kind >= static_cast<int>(kDwellings.size()))
            return false;
        if (kDwellings[static_cast<std::size_t>(kind)].empty())
            return false;
        if (type.stats().level.value() != level)
            return false;
        return !filter.has_value() || type.alignment() == *filter;
    });
    if (kept == nullptr)
        return;

    actor.model = "creature generators.";
    actor.model += kDwellings[kept->id().value()];
    actor.family = "creature_dwelling";
    actor.sub    = std::string{kept->keyword()} + "_dwelling";
}

}
