#include "h4/app/adventure/adventure_panel.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "h4/app/adventure/adventure_help.hpp"
#include "h4/assets/assets.hpp"
#include "h4/core/ascii.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/fortification.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/hero_registry.hpp"
#include "h4/model/map.hpp"
#include "h4/model/resources.hpp"
#include "h4/rules/army_movement.hpp"
#include "h4/rules/army_slots.hpp"
#include "h4/rules/army_values.hpp"
#include "h4/rules/town_progress.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/ui/grouped_amount.hpp"

namespace h4::app {

std::array<std::string, screens::kAdventureResources> purse_amounts(const model::ResourceBundle& purse) {
    using model::Resource;
    using screens::AdventureResource;
    constexpr std::array<std::pair<AdventureResource, Resource>, screens::kAdventureResources> kCounters{{
        {AdventureResource::gold, Resource::gold},
        {AdventureResource::wood, Resource::wood},
        {AdventureResource::ore, Resource::ore},
        {AdventureResource::mercury, Resource::mercury},
        {AdventureResource::sulfur, Resource::sulfur},
        {AdventureResource::crystal, Resource::crystal},
        {AdventureResource::gems, Resource::gems},
    }};
    std::array<std::string, screens::kAdventureResources> out;
    for (const auto& [counter, resource] : kCounters)
        out[std::to_underlying(counter)] = ui::grouped_amount(purse[resource].value());
    return out;
}

namespace {

const formats::Layer* face_of(const assets::Assets& assets, const model::MapActor& army) {
    if (const formats::HeroRecord* hero = army.army.leader(); hero != nullptr)
        return hero_face(assets, *hero, 52);
    for (const formats::ArmySlot& slot : army.army.slots)
        if (slot.kind == formats::ArmySlot::Kind::creature)
            return creature_face(assets, slot.stack.creature, 52);
    return nullptr;
}

constexpr std::array<std::string_view, 19> kGrounds{
    "water",    "grass", "rough", "swamp", "volcanic", "snow",     "sand",     "dirt",  "subterranean", "water",
    "volcanic", "snow",  "grass", "grass", "grass",    "volcanic", "volcanic", "grass", "grass",
};

std::string_view fortification(const formats::TownBuildings& buildings) {
    return model::to_string(model::fortification_of(buildings));
}

}

std::vector<const model::MapActor*> player_armies(const model::Map& map, std::optional<model::PlayerColour> colour) {
    std::vector<const model::MapActor*> out;
    if (!colour)
        return out;
    for (const model::MapActor* actor : map.owned_by(*colour))
        if (actor->is_army())
            out.push_back(actor);
    return out;
}

std::vector<screens::HeroListEntry> hero_list_entries(const assets::Assets& assets,
                                                      std::span<const model::MapActor* const> armies,
                                                      const model::GameDatabase* database,
                                                      const MovementLeft& left) {
    std::vector<screens::HeroListEntry> out;
    for (const model::MapActor* army : armies) {
        screens::HeroListEntry entry;
        entry.face = face_of(assets, *army);
        entry.help = army_row_help(assets, database, *army);
        if (database != nullptr) {
            entry.move = rules::army_movement(army->army, database->creatures()).day;
            if (left)
                if (const std::optional<int> today = left(*army); today.has_value())
                    entry.move = *today;
            entry.strength = rules::strength(army->army, database->creatures());
        }
        entry.move_help = army_move_help(assets, whole_tiles(entry.move));
        out.push_back(entry);
    }
    return out;
}

std::vector<const model::MapActor*> player_towns(const model::Map& map, std::optional<model::PlayerColour> colour) {
    std::vector<const model::MapActor*> out;
    if (!colour)
        return out;
    for (const model::MapActor* actor : map.owned_by(*colour))
        if (actor->kind == model::ActorKind::town)
            out.push_back(actor);
    return out;
}

std::vector<screens::TownListEntry>
town_list_entries(const assets::Assets& assets, const model::Map& map, std::span<const model::MapActor* const> towns) {
    std::vector<screens::TownListEntry> out;
    for (const model::MapActor* town : towns) {
        screens::TownListEntry entry;
        if (town->family == "town")
            entry.alignment = model::parse_alignment(town->sub);
        if (const formats::GridTile* tile = map.terrain().tile(town->position.x, town->position.y, town->level);
            tile != nullptr && tile->terrain < kGrounds.size())
            entry.ground = kGrounds[tile->terrain];
        entry.fortification = fortification(town->town_buildings);
        if (entry.alignment.has_value()) {
            const auto progress = rules::building_progress(*entry.alignment, town->town_buildings);
            for (std::size_t i = 0; i < progress.size(); ++i)
                entry.bars[i] = {progress[i].built, progress[i].total};
        }
        core::Size box{1, 1};
        if (const formats::AdvObject* model = assets.object(town->model))
            box = model->size();
        entry.tiles = core::Rect{town->position, box};
        out.push_back(entry);
    }
    return out;
}

std::vector<screens::ArmyRingEntry>
army_ring_entries(const assets::Assets& assets, const model::MapActor* army, const model::GameDatabase* database) {
    std::vector<screens::ArmyRingEntry> out(screens::kArmyRings);
    if (army == nullptr)
        return out;
    for (std::size_t i = 0; i < screens::kArmyRings && i < army->army.slots.size(); ++i) {
        const formats::ArmySlot& slot = army->army.slots[i];
        screens::ArmyRingEntry& entry = out[i];
        if (slot.kind == formats::ArmySlot::Kind::creature) {
            entry.kind  = screens::ArmyRingEntry::Kind::creature;
            entry.face  = creature_face(assets, slot.stack.creature, 52);
            entry.count = std::to_string(slot.stack.count);
        } else if (slot.kind == formats::ArmySlot::Kind::hero) {
            entry.kind   = screens::ArmyRingEntry::Kind::hero;
            entry.face   = hero_face(assets, slot.hero, 52);
            entry.health = entry.health_max = 1;
            entry.mana = entry.mana_max = 1;
        }
        entry.help = ring_help(assets, database, slot);
    }
    return out;
}

std::optional<std::size_t> row_of(std::span<const model::MapActor* const> armies, const model::MapActor* army) noexcept {
    for (std::size_t i = 0; i < armies.size(); ++i)
        if (armies[i] == army)
            return i;
    return std::nullopt;
}

std::string date_words(std::string_view pattern, const rules::Calendar& calendar) {
    return model::substituted(pattern,
                              {{"%day", std::to_string(calendar.day().value())},
                               {"%week", std::to_string(calendar.week())},
                               {"%month", std::to_string(calendar.month())}});
}

}
