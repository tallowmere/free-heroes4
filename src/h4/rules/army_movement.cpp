#include "h4/rules/army_movement.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/creature_catalog.hpp"
#include "h4/rules/army_slots.hpp"

namespace h4::rules {

namespace {

constexpr std::array<std::int32_t, 6> kPathfindingMovement{100, 100, 100, 100, 125, 150};
constexpr std::array<std::int32_t, 6> kPathfindingTerrain{0, 25, 50, 100, 100, 100};

constexpr std::array<std::uint8_t, 6> kHomeTerrain{1, 5, 4, 3, 1, 2};

[[nodiscard]] std::size_t pathfinding_row(const formats::HeroRecord& hero) {
    return static_cast<std::size_t>(hero_rank(hero, model::Discipline::pathfinding) + 1);
}

}

std::int32_t terrain_base_cost(std::uint8_t terrain) noexcept {
    constexpr std::array<std::int32_t, 19> kLadder{
        100, 100, 125, 200, 125, 175, 150, 100, 100, 150, 100, 150, 100, 100, 100, 125, 125, 100, 125
    };
    return terrain < kLadder.size() ? kLadder[terrain] : 100;
}

namespace {

struct Pace {
    std::int32_t fastest    = 0;
    std::int32_t multiplier = 100;

    [[nodiscard]] std::int32_t day(std::int32_t own) const {
        return static_cast<std::int32_t>(std::lround(own * (multiplier / 100.0)));
    }
};

Pace army_pace(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) {
    Pace out;
    out.fastest = fastest_creature_points(army, creatures);
    for (const formats::ArmySlot& slot : army.slots)
        if (slot.kind == formats::ArmySlot::Kind::hero)
            out.multiplier = std::max(out.multiplier, kPathfindingMovement[pathfinding_row(slot.hero)]);
    return out;
}

}

std::int32_t member_movement(const formats::ArmyRecord& army, std::size_t index, const model::CreatureCatalog& creatures) {
    if (index >= army.slots.size())
        return 0;
    const Pace pace = army_pace(army, creatures);
    const auto own  = member_own_points(army.slots[index], pace.fastest, creatures);
    return own.has_value() ? pace.day(*own) : 0;
}

ArmyMovement army_movement(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) {
    using Kind = formats::ArmySlot::Kind;
    ArmyMovement out;
    const Pace pace = army_pace(army, creatures);
    std::optional<std::int32_t> slowest;
    for (const formats::ArmySlot& slot : army.slots)
        if (const auto own = member_own_points(slot, pace.fastest, creatures)) {
            const std::int32_t day = pace.day(*own);
            slowest                = slowest.has_value() ? std::min(*slowest, day) : day;
        }
    out.day = slowest.value_or(0);

    for (std::uint8_t terrain = 0; terrain < out.terrain.size(); ++terrain) {
        const std::int32_t base = terrain_base_cost(terrain);
        std::int32_t best = 0;
        for (const formats::ArmySlot& slot : army.slots) {
            if (slot.kind != Kind::creature || slot.stack.count <= 0)
                continue;
            const model::CreatureType* type = slot_creature(slot, creatures);
            std::int32_t price              = base;
            if (type != nullptr && terrain < 9 && kHomeTerrain[std::to_underlying(type->alignment())] == terrain)
                price = std::max(100, base - 50);
            best = std::max(best, price);
        }
        if (best == 0)
            best = base;
        for (const formats::ArmySlot& slot : army.slots) {
            std::int32_t price = base;
            if (slot.kind == Kind::hero && base > 100)
                price = std::max(100, base - kPathfindingTerrain[pathfinding_row(slot.hero)]);
            else if (slot.kind != Kind::creature || slot.stack.count <= 0)
                continue;
            best = std::min(best, price);
        }
        out.terrain[terrain] = best;
    }
    return out;
}

}
