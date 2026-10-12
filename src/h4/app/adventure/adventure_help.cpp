#include "h4/app/adventure/adventure_help.hpp"

#include <format>
#include <optional>
#include <string>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/formats/scenario/records.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/creature_kind_registry.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/map.hpp"
#include "h4/model/skills.hpp"

namespace h4::app {

namespace {

std::string with_pronouns(std::string_view words, const formats::HeroRecord& hero) {
    const bool female = hero.sex == 0;
    return model::substituted(words, {{"%him", female ? "her" : "him"}, {"%his", female ? "her" : "his"}});
}

std::string_view class_keyword(const formats::HeroRecord& hero) {
    const model::HeroClass* klass = model::hero_class(hero);
    return klass != nullptr ? klass->keyword : std::string_view{};
}

ui::Help hero_ring(const assets::Assets& assets, const formats::HeroRecord& hero) {
    const int hit_points   = hero_hit_points(hero);
    const int spell_points = hero_spell_points(hero);
    return {std::format("{}\n{}/{} {}\n{}/{} {}",
                        hero.name,
                        hit_points,
                        hit_points,
                        assets.text("hit_points.dialog"),
                        spell_points,
                        spell_points,
                        assets.text("spell_points.dialog")),
            with_pronouns(assets.text(std::format("description_{}", class_keyword(hero))), hero)};
}

const model::CreatureType* kind_of(const model::GameDatabase* database, std::int16_t creature) {
    const std::optional<model::CreatureKindId> kind = model::creature_kind_of(creature);
    return database != nullptr && kind ? database->creatures().find(*kind) : nullptr;
}

}

ui::Help ring_help(const assets::Assets& assets, const model::GameDatabase* database, const formats::ArmySlot& slot) {
    if (slot.kind == formats::ArmySlot::Kind::hero)
        return hero_ring(assets, slot.hero);
    if (slot.kind != formats::ArmySlot::Kind::creature)
        return {};
    const model::CreatureType* type = kind_of(database, slot.stack.creature);
    if (type == nullptr)
        return {};
    return {std::format("{} {}", slot.stack.count, type->name_for(slot.stack.count)), std::string{type->long_help()}};
}

ui::Help army_row_help(const assets::Assets& assets, const model::GameDatabase* database, const model::MapActor& army) {
    if (const formats::HeroRecord* hero = army.army.leader(); hero != nullptr) {
        const model::HeroClass* klass = model::hero_class(*hero);
        const std::string align       = klass != nullptr && klass->alignment.has_value() ?
            std::string{assets.text(std::format("{}.town", model::to_string(*klass->alignment)))} :
            std::string{};
        return {hero->name,
                model::substituted(assets.text("army_list_right_click_hero.misc"),
                                   {{"%name", hero->name},
                                    {"%level", std::to_string(model::hero_level(*hero))},
                                    {"%class", std::string{assets.text(class_keyword(*hero))}},
                                    {"%align", align}})};
    }
    for (const formats::ArmySlot& slot : army.army.slots) {
        if (slot.kind != formats::ArmySlot::Kind::creature)
            continue;
        const model::CreatureType* type = kind_of(database, slot.stack.creature);
        if (type == nullptr)
            return {};
        const std::string name{type->name_for(slot.stack.count)};
        return {name,
                model::substituted(assets.text("army_list_right_click_"
                                               "creature.misc"),
                                   {{"%number", std::to_string(slot.stack.count)}, {"%name", name}})};
    }
    return {};
}

ui::Help army_move_help(const assets::Assets& assets, int tiles) {
    const assets::InterfaceHelp move = assets.help("shared", "army_ring_distance");
    return {model::substituted(move.balloon, {{"%distance", std::to_string(tiles)}}), std::string{move.right_click}};
}

}
