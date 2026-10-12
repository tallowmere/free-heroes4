#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/formats/scenario/objects.hpp"
#include "h4/model/alignment.hpp"
#include "h4/model/player_colour.hpp"
#include "h4/model/random_objects.hpp"

namespace h4::formats {
class ScenarioHeader;
}

namespace h4::model {

enum class ActorKind : std::uint8_t {
    hero,
    monster,
    town,
    mine,
    dwelling,
    resource,
    artifact,
    garrison,
    decoration,
    other
};

[[nodiscard]] std::string_view actor_kind_name(ActorKind kind) noexcept;

struct MapActor;
[[nodiscard]] bool shows_on_map(const MapActor& actor) noexcept;

struct MapActor {
    core::Point position;
    std::int32_t level = 0;
    ActorKind kind     = ActorKind::other;

    std::string family;
    std::string sub;
    std::string model;

    std::uint8_t owner = formats::kNoOwner;
    std::string name;
    formats::ArmyRecord army;
    std::optional<Alignment> alignment;
    bool starting_troops = false;
    formats::ArmyPatrol patrol;
    formats::TownBuildings town_buildings;
    std::string message;
    std::int32_t band_low = 0, band_high = 0;
    formats::SpellMask spells;
    std::uint16_t scholar_mask = 0;
    formats::NameSpec constraint;

    [[nodiscard]] bool owned() const noexcept {
        return owner < formats::kNoOwner;
    }
    [[nodiscard]] bool is_army() const noexcept {
        return model.empty();
    }
};

class Roller;

class Map {
public:
    [[nodiscard]] static core::Result<Map>
    parse(std::span<const std::byte> payload, const formats::ScenarioHeader& header, std::size_t offset);

    [[nodiscard]] const formats::ScenarioGrid& terrain() const noexcept {
        return grid_;
    }
    [[nodiscard]] std::uint32_t terrain_seed() const noexcept {
        return seed_;
    }
    [[nodiscard]] std::span<const MapActor> actors() const noexcept {
        return actors_;
    }
    [[nodiscard]] std::optional<std::uint32_t> index_of(const MapActor* actor) const noexcept;

    [[nodiscard]] std::uint16_t dimension() const noexcept {
        return grid_.width();
    }
    [[nodiscard]] std::uint8_t levels() const noexcept {
        return grid_.levels();
    }

    [[nodiscard]] std::vector<const MapActor*> actors_of(ActorKind kind) const;
    [[nodiscard]] std::vector<const MapActor*> owned_by(PlayerColour colour) const;

    void resolve_random_objects(const RandomResolution& choices, Roller& stream);
    void resolve_random_objects(const RandomResolution& choices);

private:
    void add_starting_troops(const RandomResolution& choices);
    void add_starting_buildings(Roller& stream);

    Map(formats::ScenarioGrid grid, std::uint32_t seed) noexcept : grid_{std::move(grid)}, seed_{seed} {
    }

    formats::ScenarioGrid grid_;
    std::uint32_t seed_ = 0;
    std::vector<MapActor> actors_;
};

}
