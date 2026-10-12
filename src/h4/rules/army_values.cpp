#include "h4/rules/army_values.hpp"

#include <array>
#include <bit>
#include <optional>
#include <utility>

#include "h4/model/army_member.hpp"
#include "h4/model/skills.hpp"
#include "h4/rules/army_slots.hpp"

namespace h4::rules {

int strength(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures) {
    std::int64_t value = 0;
    for (const formats::ArmySlot& slot : army.slots) {
        if (slot.kind == formats::ArmySlot::Kind::hero)
            value += static_cast<std::int64_t>(model::hero_level(slot.hero) + 1) * 100;
        else if (const model::CreatureType* type = slot_creature(slot, creatures))
            value += static_cast<std::int64_t>(type->stats().experience.value()) * slot.stack.count;
    }
    if (value < 2)
        return 0;
    return std::bit_width(static_cast<std::uint64_t>(value)) - 1;
}

namespace {

constexpr std::array<int, 4> kPenalty{0, -1, -2, -5};

}

int alignment_relation(model::Alignment a, model::Alignment b) noexcept {
    if (a == b)
        return 0;
    if (a == model::Alignment::might || b == model::Alignment::might)
        return 2;
    const int step = (std::to_underlying(a) - std::to_underlying(b) + 5) % 5;
    return step == 1 || step == 4 ? 1 : 3;
}

int member_morale(const formats::ArmyRecord& army, std::size_t slot, const model::CreatureCatalog& creatures) noexcept {
    if (slot >= army.slots.size())
        return 0;
    const std::optional<model::Alignment> own = model::member_alignment(army.slots[slot], creatures);
    if (!own.has_value())
        return 0;
    std::array<bool, 6> present{};
    for (const formats::ArmySlot& other : army.slots)
        if (const auto alignment = model::member_alignment(other, creatures))
            present[std::to_underlying(*alignment)] = true;
    int total = 1;
    for (std::size_t g = 0; g < present.size(); ++g)
        if (present[g])
            total += kPenalty[static_cast<std::size_t>(alignment_relation(*own, static_cast<model::Alignment>(g)))];
    return total;
}

}
