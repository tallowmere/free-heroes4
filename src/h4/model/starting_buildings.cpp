#include <optional>

#include "h4/model/map.hpp"
#include "h4/model/roller.hpp"

namespace h4::model {

namespace {

constexpr std::uint8_t kVillageHall = 0;
constexpr std::uint8_t kFort        = 3;
constexpr std::uint8_t kPrison      = 8;
constexpr std::uint8_t kTavern      = 9;
constexpr std::uint8_t kFirstGuild  = 20;
constexpr std::uint8_t kDwelling    = 12;

void build(formats::TownBuildings& buildings, std::uint8_t id) noexcept {
    buildings.built |= std::uint64_t{1} << id;
}

}

void Map::add_starting_buildings(Roller& roller) {
    for (MapActor& actor : actors_) {
        if (actor.kind != ActorKind::town || actor.family != "town")
            continue;
        formats::TownBuildings& buildings = actor.town_buildings;
        if (!buildings.stored) {
            if (buildings.has(kFort)) {
                build(buildings, kDwelling);
                build(buildings, kDwelling + 1);
                if (parse_alignment(actor.sub) != Alignment::might)
                    build(buildings, kFirstGuild);
            } else {
                build(buildings, static_cast<std::uint8_t>(kDwelling + roller.next(2)));
            }
            build(buildings, kTavern);
        }
        build(buildings, kVillageHall);
        build(buildings, kPrison);
    }
}

}
