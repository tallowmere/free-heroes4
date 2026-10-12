#include "h4/app/adventure/army_window_content.hpp"

#include <array>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/model/creature_abilities.hpp"
#include "h4/model/creature_kind_registry.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/hero_registry.hpp"
#include "h4/model/map.hpp"
#include "h4/model/skills.hpp"
#include "h4/rules/army_movement.hpp"
#include "h4/rules/army_slots.hpp"
#include "h4/rules/army_values.hpp"
#include "h4/rules/game_state.hpp"
#include "h4/ui/grouped_amount.hpp"

namespace h4::app {

namespace {

constexpr std::array<double, 6> kCombatTable{1.0, 1.5, 2.0, 3.0, 4.0, 4.0};
constexpr std::array<double, 6> kArcheryTable{0.7, 1.0, 1.5, 2.0, 3.0, 3.0};
constexpr std::array<double, 6> kToughnessTable{1.0, 1.5, 2.0, 3.0, 4.0, 6.0};

std::string tenths(double factor) {
    factor = std::max(factor, 0.5);
    return std::to_string(static_cast<int>(factor * 10.0 + 0.5));
}

std::string signed_number(int value) {
    return value > 0 ? std::format("+{}", value) : std::to_string(value);
}

int best_bonus(const model::MapActor& army, model::Discipline discipline, const std::array<int, 6>& table) {
    int best = 0;
    for (const formats::ArmySlot& slot : army.army.slots)
        if (slot.kind == formats::ArmySlot::Kind::hero)
            best = std::max(best, table[static_cast<std::size_t>(rules::hero_rank(slot.hero, discipline) + 1)]);
    return best;
}

constexpr std::array<int, 6> kPercentTable{0, 10, 20, 30, 40, 50};
constexpr std::array<int, 6> kTacticsTable{0, 1, 2, 3, 4, 5};

void fill_condition(screens::ArmyWindowMember& page,
                    const model::MapActor& army,
                    std::size_t slot,
                    const model::CreatureCatalog& creatures,
                    std::int32_t spent) {
    const MovementTiles tiles = movement_tiles(army.army, slot, creatures, spent);
    page.move                 = std::format("{}\n({})", tiles.left, tiles.day);
    const int morale          = rules::member_morale(army.army, slot, creatures);
    page.morale               = signed_number(morale);
    page.morale_value         = morale;
    page.luck                 = signed_number(0);
    page.luck_value           = 0;
}

void fill_stack_numbers(screens::ArmyWindowMember& page,
                        const assets::Assets& assets,
                        const model::MapActor& army,
                        const model::CreatureType& type) {
    const model::CreatureStats& stats = type.stats();
    page.damage     = std::format("{}-{}", std::max(1, stats.damage.low), std::max(1, stats.damage.high));
    page.hit_points = std::to_string(stats.hit_points.value());
    const double attack = stats.attack * 0.1 * (100 + best_bonus(army, model::Discipline::offense, kPercentTable)) * 0.01;
    const double defense = stats.defense * 0.1 * (100 + best_bonus(army, model::Discipline::defense, kPercentTable)) * 0.01;
    const bool ranged   = model::has_ability(type.id(), "ranged");
    const bool halved   = ranged && !model::has_ability(type.id(), "normal_melee");
    page.melee_attack   = tenths(halved ? attack * 0.5 : attack);
    page.melee_defense  = tenths(defense);
    page.ranged_defense = tenths(defense);
    const std::string none{assets.text("not_applicable.dialog")};
    page.ranged_attack = ranged ? tenths(attack) : none;
    page.shots = model::has_ability(type.id(), "unlimited_shots") ? std::string{assets.text("infinity_abbreviation.misc")} :
        ranged ? std::to_string(stats.shots) :
                 none;
    page.speed = std::to_string(stats.speed + best_bonus(army, model::Discipline::tactics, kTacticsTable));
    if (stats.spell_points > 0)
        page.spell_points = std::to_string(stats.spell_points);
}

screens::ArmyWindowMember creature_page(const assets::Assets& assets,
                                        const model::MapActor& army,
                                        std::size_t slot,
                                        const model::GameDatabase& database,
                                        std::int32_t spent) {
    screens::ArmyWindowMember page;
    page.hero                           = false;
    const formats::CreatureStack& stack = army.army.slots[slot].stack;
    const model::CreatureType* type     = rules::slot_creature(army.army.slots[slot], database.creatures());
    if (type == nullptr)
        return page;
    page.name      = std::format("{} {}", stack.count, type->name_for(stack.count));
    page.text      = type->long_help();
    page.model     = model::creature_kind_keyword(type->id());
    page.alignment = model::to_string(type->alignment());
    page.portrait  = creature_face(assets, stack.creature, 82);
    fill_stack_numbers(page, assets, army, *type);
    fill_condition(page, army, slot, database.creatures(), spent);
    std::size_t at = 0;
    for (const std::string_view ability : model::creature_abilities(type->id())) {
        if (at >= 20)
            break;
        page.skills[at / 4][at % 4] = ability_icon(assets, database, ability);
        ++at;
    }
    return page;
}

void fill_skills(screens::ArmyWindowMember& hero,
                 const assets::Assets& assets,
                 const formats::HeroRecord& record,
                 const model::GameDatabase* database) {
    std::array<int, model::kSchools> row_of{};
    row_of.fill(-1);
    std::array<std::size_t, 5> used{};
    int rows = 0;
    for (const model::HeroSkill& skill : model::hero_skills(record)) {
        int& row = row_of[std::to_underlying(model::school_head(skill.discipline))];
        if (row < 0) {
            if (rows == 5)
                continue;
            row = rows++;
        }
        std::size_t& column = used[static_cast<std::size_t>(row)];
        if (column >= 4)
            continue;
        hero.skills[static_cast<std::size_t>(row)][column++] = skill_icon(assets, database, skill);
    }
}

void fill_inventory(screens::ArmyWindowMember& hero,
                    const assets::Assets& assets,
                    const rules::HeroState& state,
                    const model::GameDatabase& database) {
    const auto icon_of = [&](const rules::Artifact& artifact) {
        screens::NamedIcon icon;
        const std::optional<model::ArtifactId> id = model::artifact_id_of(artifact.instance.id);
        if (const model::ArtifactType* type = id ? database.artifacts().find(*id) : nullptr) {
            icon.base = artifact_icon(assets, *type);
            icon.help = {type->name, type->help};
        }
        return icon;
    };
    for (std::size_t i = 0; i < hero.worn.size(); ++i)
        if (state.inventory.equipment[i].has_value())
            hero.worn[i] = icon_of(*state.inventory.equipment[i]);
    for (std::size_t i = 0; i < hero.backpack.size() && i < state.inventory.backpack.size(); ++i)
        hero.backpack[i] = icon_of(state.inventory.backpack[i]);
}

void fill_hero_numbers(screens::ArmyWindowMember& hero, const assets::Assets& assets, const formats::HeroRecord& record) {
    const int combat    = rules::hero_rank(record, model::Discipline::combat);
    const int archery   = rules::hero_rank(record, model::Discipline::archery);
    const int toughness = rules::hero_rank(record, model::Discipline::toughness);
    hero.melee_attack   = tenths(kCombatTable[static_cast<std::size_t>(combat + 1)] + record.additions[0] * 0.1);
    const std::string defense = tenths(kToughnessTable[static_cast<std::size_t>(toughness + 1)] + record.additions[1] * 0.1);
    hero.melee_defense  = defense;
    hero.ranged_defense = defense;
    const std::string none{assets.text("not_applicable.dialog")};
    hero.ranged_attack = archery >= 0 ?
        tenths(kArcheryTable[static_cast<std::size_t>(archery + 1)] + record.additions[0] * 0.07) :
        none;
    hero.shots         = archery >= 0 ? std::to_string(archery > 3 ? 24 : 12) : none;
    hero.speed         = std::to_string(std::max(1, 6 + record.additions[2]));
}

const formats::BitmapGroup* hero_doll(const assets::Assets& assets, const formats::HeroRecord& record) {
    const model::HeroEntry* entry = model::hero_entry(record.id);
    if (entry == nullptr)
        return nullptr;
    const model::HeroClass* klass = model::hero_class(record);
    const model::Alignment alignment = klass != nullptr && klass->alignment.has_value() ? *klass->alignment : entry->alignment;
    return assets.group(std::format("dialog.army.{}_{}_{}",
                                    model::to_string(alignment),
                                    klass != nullptr && klass->magic ? "magic" : "might",
                                    record.sex == 0 ? "female" : "male"));
}

screens::ArmyWindowMember hero_page(const assets::Assets& assets,
                                    const model::MapActor& army,
                                    std::size_t selected,
                                    const rules::HeroState* state,
                                    const model::GameDatabase* database,
                                    std::int32_t spent) {
    const formats::HeroRecord& record = army.army.slots[selected].hero;
    const screens::StatEntry strip    = strip_entry(assets, &army, selected, database, spent);
    screens::ArmyWindowMember hero;
    hero.name       = record.name;
    hero.class_line = strip.class_line;
    hero.hit_points = strip.health;
    hero.damage     = strip.damage;
    const int mana    = hero_spell_points(record);
    hero.spell_points = std::format("{}\n({})", mana, mana);
    hero.luck         = signed_number(0);
    if (database != nullptr)
        fill_condition(hero, army, selected, database->creatures(), spent);
    fill_hero_numbers(hero, assets, record);
    hero.experience = ui::grouped_amount(state != nullptr ? state->experience : 0);
    hero.portrait   = hero_face(assets, record, 82);
    hero.doll       = hero_doll(assets, record);
    fill_skills(hero, assets, record, database);
    if (state != nullptr && database != nullptr)
        fill_inventory(hero, assets, *state, *database);
    return hero;
}

}

screens::ArmyWindowModel army_window_content(const assets::Assets& assets,
                                             const model::MapActor& army,
                                             std::optional<std::size_t> selected,
                                             const rules::HeroState* state,
                                             const model::GameDatabase* database,
                                             std::int32_t spent) {
    screens::ArmyWindowModel out;
    const auto rings = army_ring_entries(assets, &army, database);
    for (std::size_t i = 0; i < out.army.size() && i < rings.size(); ++i)
        out.army[i] = rings[i];
    out.below.emplace();
    out.selected = selected;
    if (!selected || *selected >= army.army.slots.size())
        return out;
    const formats::ArmySlot& slot = army.army.slots[*selected];
    if (slot.kind == formats::ArmySlot::Kind::creature && database != nullptr)
        out.member = creature_page(assets, army, *selected, *database, spent);
    else if (slot.kind == formats::ArmySlot::Kind::hero)
        out.member = hero_page(assets, army, *selected, state, database, spent);
    return out;
}

}
