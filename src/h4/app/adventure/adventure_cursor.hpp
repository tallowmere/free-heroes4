#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "h4/core/geometry.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/rules/adventure_board.hpp"
#include "h4/rules/adventure_path.hpp"
#include "h4/rules/enemy_zones.hpp"
#include "h4/scene/map_hover.hpp"

namespace h4::model {
class Map;
class Shroud;
}

namespace h4::app {

struct MapMover {
    std::uint32_t actor            = 0;
    const rules::PathField* field  = nullptr;
    std::int32_t day               = 0;
    std::int32_t left              = 0;
    const rules::EnemyZones* zones = nullptr;
};

using Friendly = std::function<bool(std::uint8_t a, std::uint8_t b)>;

struct CursorWorld {
    const model::Map& map;
    const rules::AdventureBoard& board;
    Friendly friendly;
    std::function<bool(core::Point)> explored;
};

[[nodiscard]] rules::PathField::Rules
land_search_rules(const rules::AdventureBoard& board, std::uint32_t mover, const model::Shroud* seen);

[[nodiscard]] std::optional<std::uint32_t> object_under(const CursorWorld& world, const scene::MapHover& hover);

struct MoveTarget {
    rules::MapPoint to;
    std::int32_t total = 0;
};
[[nodiscard]] std::optional<MoveTarget> move_target(const CursorWorld& world, const MapMover& mover, const scene::MapHover& hover);

[[nodiscard]] cursor::CursorState
map_cursor(const CursorWorld& world, const std::optional<MapMover>& mover, const scene::MapHover& hover);

}
