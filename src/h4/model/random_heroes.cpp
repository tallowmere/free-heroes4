#include <array>

#include "h4/model/hero_class.hpp"
#include "h4/model/hero_table.hpp"
#include "h4/model/map.hpp"
#include "h4/model/random_objects.hpp"
#include "h4/model/random_passes.hpp"

namespace h4::model {

namespace {

constexpr std::uint8_t kBasicClasses = 11;

}

HeroPool::HeroPool(const HeroTable* heroes, std::span<const MapActor> actors) : heroes_{heroes} {
    if (heroes_ == nullptr)
        return;
    uses_.assign(heroes_->rows().size(), 0);
    for (const MapActor& actor : actors)
        for (const formats::ArmySlot& slot : actor.army.slots) {
            if (slot.kind != formats::ArmySlot::Kind::hero || slot.hero.id < 0)
                continue;
            const auto rows = heroes_->rows();
            for (std::size_t i = 0; i < rows.size(); ++i)
                if (rows[i].registry_id == slot.hero.id) {
                    ++uses_[i];
                    break;
                }
        }
}

const HeroRow* HeroPool::named_row(int registry_id) const noexcept {
    if (heroes_ == nullptr)
        return nullptr;
    for (const HeroRow& row : heroes_->rows())
        if (!row.generic && row.registry_id == registry_id)
            return &row;
    return nullptr;
}

std::optional<std::size_t> HeroPool::draw(std::optional<HeroClassId> hero_class, int sex, Roller& roller) {
    const auto rows = heroes_->rows();
    const auto least_used_of = [&](auto fits) {
        std::vector<std::size_t> admitted;
        for (std::size_t i = 0; i < rows.size(); ++i)
            if (fits(rows[i]))
                admitted.push_back(i);
        return least_used(std::span<const std::size_t>{admitted}, [&](std::size_t i) { return uses_[i]; });
    };

    auto [lowest, pool] = least_used_of([&](const HeroRow& row) {
        return !row.generic && row.allowed && hero_class && row.hero_class == hero_class && row.sex == sex;
    });
    if (pool.empty() || lowest != 0)
        pool = least_used_of([&](const HeroRow& row) { return row.generic && row.sex == sex; }).second;
    if (pool.empty())
        return std::nullopt;
    const std::size_t row = pool[roller.next_index(pool.size())];
    ++uses_[row];
    return row;
}

void HeroPool::resolve(MapActor& actor, const RandomResolution& choices, Roller& roller) {
    const std::optional<Alignment> alignment = owner_alignment(actor, choices);
    for (formats::ArmySlot& slot : actor.army.slots) {
        if (slot.kind != formats::ArmySlot::Kind::hero)
            continue;
        if (slot.hero.id >= 0) {
            if (const HeroRow* row = named_row(slot.hero.id); row != nullptr) {
                if (row->hero_class.has_value())
                    slot.hero.hero_class = static_cast<std::int8_t>(row->hero_class->value());
                slot.hero.sex = static_cast<std::int8_t>(row->sex);
                if (slot.hero.name.empty())
                    slot.hero.name = row->name;
            }
            continue;
        }
        if (!hero_class_id(slot.hero)) {
            std::array<HeroClassId, kBasicClasses> fitting{};
            int count = 0;
            for (std::uint8_t id = 0; id < kBasicClasses; ++id) {
                const HeroClass* record = hero_class(HeroClassId{id});
                if (record == nullptr)
                    continue;
                if (alignment.has_value() && record->alignment != alignment)
                    continue;
                fitting[static_cast<std::size_t>(count++)] = HeroClassId{id};
            }
            if (count > 0)
                slot.hero.hero_class = static_cast<std::int8_t>(fitting[static_cast<std::size_t>(roller.next(count))].value());
        }
        if (slot.hero.sex < 0)
            slot.hero.sex = static_cast<std::int8_t>(roller.next(2));

        if (heroes_ == nullptr)
            continue;
        const std::optional<std::size_t> row = draw(hero_class_id(slot.hero), slot.hero.sex, roller);
        if (!row.has_value())
            continue;
        const HeroRow& chosen = heroes_->rows()[*row];
        slot.hero.id = static_cast<std::int16_t>(chosen.registry_id.value_or(-1));
        if (slot.hero.name.empty())
            slot.hero.name = chosen.name;
    }
    if (const formats::HeroRecord* leader = actor.army.leader(); leader != nullptr)
        actor.name = leader->name;
}

}
