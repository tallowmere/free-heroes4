#include "h4/model/map.hpp"

#include <array>
#include <functional>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/formats/scenario/header.hpp"

namespace h4::model {

namespace {

constexpr std::array<std::string_view, 10> kKindNames{"hero", "monster", "town", "mine", "dwelling", "resource", "artifact", "garrison", "decoration", "other"};

constexpr std::array<std::string_view, 10>
    kDecorations{"decorative", "nothing", "system_object", "bridge", "rainbow", "buoy", "victory_flag", "chain", "corpse", "clover_field"};

constexpr std::array<std::string_view, 8> kResources{"material_pile", "random_material_pile", "treasure", "campfire", "flotsam", "magic_gem", "corpse", "medicine_wagon"};

constexpr std::array<std::string_view, 5> kArtifacts{"artifact", "random_artifact", "random_potion", "random_staff", "random_tome"};

[[nodiscard]] bool in(std::span<const std::string_view> names, std::string_view family) {
    for (std::string_view name : names)
        if (core::iequals(name, family))
            return true;
    return false;
}

[[nodiscard]] ActorKind kind_of(const formats::ScenarioObject& object) {
    if (object.is_army)
        return object.hero() != nullptr ? ActorKind::hero : ActorKind::monster;
    switch (object.body) {
    case formats::ObjectBody::town:
    case formats::ObjectBody::random_town: return ActorKind::town;
    case formats::ObjectBody::abandoned_mine: return ActorKind::mine;
    case formats::ObjectBody::weekly_generator:
    case formats::ObjectBody::random_dwelling: return ActorKind::dwelling;
    case formats::ObjectBody::random_monster: return ActorKind::monster;
    case formats::ObjectBody::prison: return ActorKind::hero;
    case formats::ObjectBody::ownable:
        return core::iequals(object.family, "garrison") ? ActorKind::garrison : ActorKind::mine;
    default: break;
    }
    if (in(kResources, object.family))
        return ActorKind::resource;
    if (in(kArtifacts, object.family))
        return ActorKind::artifact;
    if (in(kDecorations, object.family))
        return ActorKind::decoration;
    return ActorKind::other;
}

}

std::string_view actor_kind_name(ActorKind kind) noexcept {
    const auto index = std::to_underlying(kind);
    return index < kKindNames.size() ? kKindNames[index] : std::string_view{};
}

bool shows_on_map(const MapActor& actor) noexcept {
    return actor.family != "event_trigger";
}

core::Result<Map> Map::parse(std::span<const std::byte> payload, const formats::ScenarioHeader& header, std::size_t offset) {
    const auto objects = formats::ScenarioObjects::parse(payload, offset, header);
    if (!objects)
        return std::unexpected{objects.error()};

    auto grid = formats::ScenarioGrid::parse(payload, objects->end(), header.grid_dimension(), header.levels(), header.version());
    if (!grid)
        return std::unexpected{grid.error()};

    Map map{std::move(*grid), header.terrain_seed()};
    map.actors_.reserve(objects->objects().size());
    for (const formats::ScenarioObject& object : objects->objects()) {
        MapActor actor;
        actor.position        = {object.x, object.y};
        actor.level           = object.level;
        actor.kind            = kind_of(object);
        actor.family          = object.family;
        actor.sub             = object.sub;
        actor.model           = object.model;
        actor.owner           = object.owner;
        actor.army            = object.army;
        actor.alignment       = alignment_of(object.alignment);
        actor.starting_troops = object.starting_troops;
        actor.patrol          = object.patrol;
        actor.message         = object.message;
        actor.constraint      = object.name_spec;
        actor.band_low        = object.band_low;
        actor.band_high       = object.band_high;
        actor.scholar_mask    = object.scholar_mask;
        actor.spells          = object.spells;
        if (object.is_town()) {
            actor.name           = object.town_name;
            actor.town_buildings = object.town_buildings;
        } else if (const formats::HeroRecord* hero = object.hero(); hero != nullptr) {
            actor.name = hero->name;
        }
        map.actors_.push_back(std::move(actor));
    }
    return map;
}

std::optional<std::uint32_t> Map::index_of(const MapActor* actor) const noexcept {
    const std::less<const MapActor*> before;
    const MapActor* const first = actors_.data();
    if (actor == nullptr || before(actor, first) || !before(actor, first + actors_.size()))
        return std::nullopt;
    return static_cast<std::uint32_t>(actor - first);
}

std::vector<const MapActor*> Map::actors_of(ActorKind kind) const {
    std::vector<const MapActor*> out;
    for (const MapActor& actor : actors_)
        if (actor.kind == kind)
            out.push_back(&actor);
    return out;
}

std::vector<const MapActor*> Map::owned_by(PlayerColour colour) const {
    std::vector<const MapActor*> out;
    for (const MapActor& actor : actors_)
        if (actor.owned() && actor.owner == std::to_underlying(colour))
            out.push_back(&actor);
    return out;
}

}
