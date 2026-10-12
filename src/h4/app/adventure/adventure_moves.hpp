#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "h4/app/adventure/adventure_cursor.hpp"
#include "h4/core/geometry.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/rules/adventure_board.hpp"
#include "h4/rules/adventure_path.hpp"
#include "h4/rules/enemy_zones.hpp"
#include "h4/scene/map_path.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::model {
class GameDatabase;
class Map;
struct MapActor;
class ScenarioSetup;
class Shroud;
}
namespace h4::app {

[[nodiscard]] rules::AdventureBoard board_of(const assets::Assets& assets, const model::Map& map);

class AdventureMoves {
public:
    AdventureMoves(const assets::Assets& assets,
                   const model::Map& map,
                   const model::ScenarioSetup& setup,
                   const model::GameDatabase* database,
                   const model::Shroud* seen);
    AdventureMoves(const AdventureMoves&)            = delete;
    AdventureMoves& operator=(const AdventureMoves&) = delete;

    void select(const model::MapActor* army, std::int32_t left = std::numeric_limits<std::int32_t>::max());

    [[nodiscard]] std::optional<std::uint32_t> object_at(const scene::MapHover& hover) const;
    [[nodiscard]] cursor::CursorState cursor_at(const scene::MapHover& hover) const;

    struct Click {
        std::vector<scene::PathMark> marks;
        bool move = false;
    };
    [[nodiscard]] Click click(const scene::MapHover& hover);

    [[nodiscard]] std::vector<scene::PathMark> keep_path(const rules::MapPoint& to);
    [[nodiscard]] const std::optional<rules::MapPoint>& destination() const noexcept {
        return destination_;
    }

    [[nodiscard]] std::optional<std::int32_t> path_cost() const;

    struct Walk {
        std::vector<rules::PathField::PathStep> steps;
        bool out_of_movement = false;
        bool visit           = false;
        std::optional<rules::PathField::PathStep> visit_step;
        std::uint32_t visited = 0;
    };
    [[nodiscard]] Walk walk() const;

    [[nodiscard]] std::span<const std::uint32_t> objects_at(const rules::MapPoint& at) const noexcept {
        return board_.objects(at);
    }
    void object_taken(std::uint32_t actor) {
        board_.remove(actor);
    }
    void army_moved(std::uint32_t actor, const rules::MapPoint& to);
    [[nodiscard]] rules::MapPoint position(std::uint32_t actor) const noexcept {
        return board_.origin(actor);
    }

private:
    [[nodiscard]] std::vector<scene::PathMark> marks_to(const rules::MapPoint& to) const;
    [[nodiscard]] bool friendly(std::uint8_t a, std::uint8_t b) const noexcept;
    [[nodiscard]] CursorWorld world(std::int32_t level) const;
    [[nodiscard]] std::optional<MapMover> mover() const;
    [[nodiscard]] std::vector<std::uint32_t> visible_enemies(std::uint8_t owner) const;
    void mark_danger(const std::vector<std::uint32_t>& enemies);

    const model::Map* map_;
    const model::GameDatabase* database_;
    const model::Shroud* seen_;
    rules::AdventureBoard board_;
    std::array<std::size_t, 6> teams_{};
    std::array<bool, 6> humans_{};
    std::optional<std::uint32_t> selected_;
    std::optional<rules::EnemyZones> zones_;
    std::optional<rules::PathField> field_;
    std::vector<bool> danger_;
    std::int32_t day_  = 0;
    std::int32_t left_ = 0;
    std::optional<rules::MapPoint> destination_;
};

}
