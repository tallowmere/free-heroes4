#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/model/player_colour.hpp"
#include "h4/rules/calendar.hpp"
#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/adventure/army_rings_view.hpp"
#include "h4/screens/adventure/hero_list_view.hpp"
#include "h4/screens/adventure/stat_strip.hpp"
#include "h4/screens/adventure/town_list_view.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
struct ArmyRecord;
struct HeroRecord;
}
namespace h4::model {
struct ArtifactType;
class CreatureCatalog;
struct HeroSkill;
class GameDatabase;
class Map;
struct MapActor;
class ResourceBundle;
}
namespace h4::screens {
class AdventureView;
}

namespace h4::app {

[[nodiscard]] std::array<std::string, screens::kAdventureResources> purse_amounts(const model::ResourceBundle& purse);

[[nodiscard]] std::vector<const model::MapActor*> player_armies(const model::Map& map, std::optional<model::PlayerColour> colour);
using MovementLeft = std::function<std::optional<int>(const model::MapActor& army)>;
[[nodiscard]] std::vector<screens::HeroListEntry> hero_list_entries(const assets::Assets& assets,
                                                                    std::span<const model::MapActor* const> armies,
                                                                    const model::GameDatabase* database,
                                                                    const MovementLeft& left = {});

[[nodiscard]] std::optional<std::size_t> row_of(std::span<const model::MapActor* const> armies, const model::MapActor* army) noexcept;

[[nodiscard]] std::vector<const model::MapActor*> player_towns(const model::Map& map, std::optional<model::PlayerColour> colour);
[[nodiscard]] std::vector<screens::TownListEntry>
town_list_entries(const assets::Assets& assets, const model::Map& map, std::span<const model::MapActor* const> towns);

[[nodiscard]] std::vector<screens::ArmyRingEntry>
army_ring_entries(const assets::Assets& assets, const model::MapActor* army, const model::GameDatabase* database = nullptr);

[[nodiscard]] int hero_spell_points(const formats::HeroRecord& hero);
[[nodiscard]] int hero_hit_points(const formats::HeroRecord& hero);

[[nodiscard]] std::optional<std::size_t> default_ring(const model::MapActor* army);

[[nodiscard]] constexpr int whole_tiles(std::int32_t points) noexcept {
    return points / 100;
}

struct MovementTiles {
    int left = 0;
    int day  = 0;
};
[[nodiscard]] MovementTiles
movement_tiles(const formats::ArmyRecord& army, std::size_t slot, const model::CreatureCatalog& creatures, std::int32_t spent);

[[nodiscard]] screens::StatEntry strip_entry(const assets::Assets& assets,
                                             const model::MapActor* army,
                                             std::size_t ring,
                                             const model::GameDatabase* database,
                                             std::int32_t spent = 0);

[[nodiscard]] const formats::Layer* hero_face(const assets::Assets& assets, const formats::HeroRecord& hero, int size);
[[nodiscard]] const formats::Layer* creature_face(const assets::Assets& assets, std::int16_t creature, int size);

[[nodiscard]] const formats::Layer* artifact_icon(const assets::Assets& assets, const model::ArtifactType& type);

[[nodiscard]] screens::NamedIcon
skill_icon(const assets::Assets& assets, const model::GameDatabase* database, const model::HeroSkill& skill);

[[nodiscard]] screens::NamedIcon
ability_icon(const assets::Assets& assets, const model::GameDatabase& database, std::string_view ability);

[[nodiscard]] std::string date_words(std::string_view pattern, const rules::Calendar& calendar);

}
