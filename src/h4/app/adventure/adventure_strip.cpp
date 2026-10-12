#include <array>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/creature_abilities.hpp"
#include "h4/model/creature_kind_registry.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/map.hpp"
#include "h4/model/skills.hpp"
#include "h4/rules/army_movement.hpp"
#include "h4/rules/army_slots.hpp"
#include "h4/rules/army_values.hpp"

namespace h4::app {

namespace {

constexpr std::array kManaDisciplines{model::Discipline::healing,
                                      model::Discipline::enchantment,
                                      model::Discipline::black,
                                      model::Discipline::conjuration,
                                      model::Discipline::herbalism};

int own_tiles(std::int32_t move) {
    return whole_tiles(rules::own_movement_points(move));
}

std::string
movement_line(const formats::ArmyRecord& army, std::size_t slot, const model::GameDatabase* database, std::int32_t spent, int own) {
    if (database == nullptr)
        return std::format("{} / {}", own, own);
    const MovementTiles tiles = movement_tiles(army, slot, database->creatures(), spent);
    return std::format("{} / {}", tiles.left, tiles.day);
}

}

MovementTiles
movement_tiles(const formats::ArmyRecord& army, std::size_t slot, const model::CreatureCatalog& creatures, std::int32_t spent) {
    const std::int32_t day = rules::member_movement(army, slot, creatures);
    return {whole_tiles(std::max(0, day - spent)), whole_tiles(day)};
}

int hero_hit_points(const formats::HeroRecord& hero) {
    return 10 * model::hero_level(hero) + 90;
}

int hero_spell_points(const formats::HeroRecord& hero) {
    int mana = 10;
    for (const model::HeroSkill& skill : model::hero_skills(hero))
        if (std::ranges::find(kManaDisciplines, skill.discipline) != kManaDisciplines.end())
            mana += 10 * (skill.rank + 1);
    return mana;
}

std::optional<std::size_t> default_ring(const model::MapActor* army) {
    if (army == nullptr)
        return std::nullopt;
    std::optional<std::size_t> first;
    for (std::size_t i = 0; i < army->army.slots.size(); ++i) {
        const formats::ArmySlot& slot = army->army.slots[i];
        if (slot.kind == formats::ArmySlot::Kind::hero)
            return i;
        if (slot.kind == formats::ArmySlot::Kind::creature && !first)
            first = i;
    }
    return first;
}

namespace {

void fill_hero(screens::StatEntry& out,
               const assets::Assets& assets,
               const formats::ArmyRecord& army,
               std::size_t ring,
               const model::GameDatabase* database,
               std::int32_t spent) {
    const formats::HeroRecord& hero = army.slots[ring].hero;
    const int level                 = model::hero_level(hero);
    out.name                        = hero.name;
    if (const model::HeroClass* klass = model::hero_class(hero))
        out.class_line = model::substituted(assets.text("hero_level.dialog"),
                                            {{"%level", std::to_string(level)},
                                             {"%class_name", std::string{assets.text(klass->keyword)}}});
    out.health = std::to_string(hero_hit_points(hero));
    out.damage = std::format("{}-{}", 16 + 16 * (level - 1) / 10, 24 + 24 * (level - 1) / 10);
    out.move   = movement_line(army, ring, database, spent, own_tiles(rules::kHeroMove));
    const std::vector<model::HeroSkill> skills = model::panel_order(model::hero_skills(hero));
    const int mana                             = hero_spell_points(hero);
    out.mana                                   = std::format("{} / {}", mana, mana);
    for (std::size_t i = 0; i < out.icons.size() && i < skills.size(); ++i)
        out.icons[i] = skill_icon(assets, database, skills[i]);
}

void fill_stack(screens::StatEntry& out,
                const assets::Assets& assets,
                const formats::ArmyRecord& army,
                std::size_t ring,
                const model::GameDatabase& database,
                std::int32_t spent) {
    const formats::ArmySlot& slot   = army.slots[ring];
    const model::CreatureType* type = rules::slot_creature(slot, database.creatures());
    if (type == nullptr)
        return;
    out.name                          = std::format("{} {}", slot.stack.count, type->name_for(slot.stack.count));
    const model::CreatureStats& stats = type->stats();
    out.health                        = std::to_string(stats.hit_points.value());
    out.damage                        = std::format("{}-{}", stats.damage.low, stats.damage.high);
    out.move                          = movement_line(army, ring, &database, spent, own_tiles(stats.movement));
    out.mana                          = std::format("{} / {}", stats.spell_points, stats.spell_points);
    const std::span<const std::string_view> abilities = model::creature_abilities(type->id());
    for (std::size_t i = 0; i < out.icons.size() && i < abilities.size(); ++i)
        out.icons[i] = ability_icon(assets, database, abilities[i]);
}

}

screens::StatEntry
strip_entry(const assets::Assets& assets, const model::MapActor* army, std::size_t ring, const model::GameDatabase* database, std::int32_t spent) {
    screens::StatEntry out;
    if (army == nullptr || ring >= army->army.slots.size())
        return out;
    const formats::ArmySlot& slot = army->army.slots[ring];
    if (slot.kind == formats::ArmySlot::Kind::empty)
        return out;
    out.shown = true;
    out.luck = 0;
    if (database != nullptr)
        out.morale = rules::member_morale(army->army, ring, database->creatures());
    if (slot.kind == formats::ArmySlot::Kind::hero)
        fill_hero(out, assets, army->army, ring, database, spent);
    else if (database != nullptr)
        fill_stack(out, assets, army->army, ring, *database, spent);
    return out;
}

}
