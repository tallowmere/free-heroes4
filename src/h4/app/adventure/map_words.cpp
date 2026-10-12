#include "h4/app/adventure/map_words.hpp"

#include <array>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "h4/assets/assets.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/formats/scenario/records.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/creature_kind_registry.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/map.hpp"
#include "h4/model/shroud.hpp"

namespace h4::app {

namespace {

constexpr std::array<std::array<std::string_view, 2>, formats::kTerrainClasses> kTerrainKeys{{
    {"water_1", "water_2"},
    {"grass_1", "grass_2"},
    {"rough_1", "rough_2"},
    {"swamp_1", "swamp_2"},
    {"volcanic_1", "volcanic_2"},
    {"snow_1", "snow_2"},
    {"sand_1", "sand_2"},
    {"dirt_1", "dirt_2"},
    {"subterranean_1", "subterranean_2"},
    {"water_river", "water_river"},
    {"lava_river", "lava_river"},
    {"ice_river", "ice_river"},
    {"all.special", "all.special"},
    {"life.special", "life.special"},
    {"order.special", "order.special"},
    {"death.special", "death.special"},
    {"chaos.special", "chaos.special"},
    {"nature.special", "nature.special"},
    {"combat.special", "combat.special"},
}};
constexpr std::array<std::string_view, 4> kRoadKeys{"road.none.terrain", "Road_1", "Road_2", "Road_3"};
constexpr std::array<std::string_view, 3> kRockKeys{"rock.none.terrain", "rock.cave.terrain", "rock.stone.terrain"};

std::string_view ground_key(const formats::GridTile& tile) noexcept {
    if (tile.road != 0 && tile.road < kRoadKeys.size())
        return kRoadKeys[tile.road];
    if (tile.misc != 0 && tile.misc < kRockKeys.size())
        return kRockKeys[tile.misc];
    if (tile.terrain >= kTerrainKeys.size())
        return {};
    return kTerrainKeys[tile.terrain][tile.variant & 1U];
}

std::string coordinates(const assets::Assets& assets, core::Point tile) {
    return model::substituted_in_order(assets.text("adventure_coordinates.misc"),
                                       "%i",
                                       {std::to_string(tile.x), std::to_string(tile.y)});
}

std::string object_name(const model::MapActor& actor, const model::GameDatabase* database) {
    if (!actor.name.empty())
        return actor.name;
    if (const formats::HeroRecord* hero = actor.army.leader(); hero != nullptr)
        return hero->name;
    if (database == nullptr)
        return {};
    if (actor.is_army()) {
        for (const formats::ArmySlot& slot : actor.army.slots) {
            const std::optional<model::CreatureKindId> kind = model::creature_kind_of(slot.stack.creature);
            if (slot.kind != formats::ArmySlot::Kind::creature || !kind)
                continue;
            const model::CreatureType* type = database->creatures().find(*kind);
            if (type == nullptr)
                return {};
            return std::format("{} {}", slot.stack.count, type->name_for(slot.stack.count));
        }
        return {};
    }
    return std::string{database->texts().text(actor.family, actor.sub, {}, "name")};
}

}

ui::Help map_words(const assets::Assets& assets,
                   const model::GameDatabase* database,
                   const model::Map& map,
                   const model::Shroud* seen,
                   const scene::MapHover& hover,
                   std::optional<std::uint32_t> object,
                   bool show_coordinates) {
    const std::string unexplored{assets.text("unexplored.misc")};
    const std::optional<core::Point>& tile = hover.tile;
    if (!tile)
        return {unexplored, std::string{assets.text("off_the_map.description")}};
    if (seen != nullptr && seen->level(tile->x, tile->y, hover.level) == model::kFogHidden)
        return {unexplored, std::string{assets.text("unexplored.description")}};

    const std::string prefix = show_coordinates ? coordinates(assets, *tile) : std::string{};
    if (object && *object < map.actors().size())
        if (const model::MapActor& actor = map.actors()[*object]; model::shows_on_map(actor))
            if (std::string name = object_name(actor, database); !name.empty())
                return {prefix + name, {}};
    const formats::GridTile* ground = map.terrain().tile(tile->x, tile->y, hover.level);
    if (ground == nullptr)
        return {};
    const std::string_view key = ground_key(*ground);
    return {prefix + std::string{assets.text(key)}, std::string{assets.text(std::format("{}.description", key))}};
}

}
