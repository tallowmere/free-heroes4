#include "h4/cursor/cursor_state.hpp"

#include <array>
#include <utility>

namespace h4::cursor {

namespace {

constexpr std::array<std::string_view, kCursorKindCount> kGroupNames{
    "",
    "cursor.move",
    "cursor.activate",
    "cursor.attack",
    "cursor.exchange",
    "cursor.Anchor",
    "cursor.Boat_Move",
    "cursor.Boat_Activate",
    "cursor.Danger_Zone",
    "cursor.Scuttle_Boat",
    "cursor.blocked",
    "cursor.wait",
    "cursor.split",
    "cursor.scroll",
    "cursor.combat.normal",
    "cursor.combat.walk",
    "cursor.combat.fly",
    "cursor.combat.shoot",
    "cursor.combat.melee",
    "cursor.combat.melee",
    "cursor.combat.arrow",
    "cursor.combat.info",
    "",
    "cursor.combat.no_cast",
    "cursor.combat.tower",
    "cursor.combat.displacement",
    "cursor.combat.teleport",
    "cursor.combat.quicksand",
    "cursor.combat.martyr",
    "cursor.combat.martyr_client",
    "cursor.combat.sacrifice",
    "cursor.combat.sacrifice_client",
    "cursor.combat.attack_Gate",
};

}

std::string_view group_name(CursorKind kind) noexcept {
    const auto i = std::to_underlying(kind);
    return i < kGroupNames.size() ? kGroupNames[i] : std::string_view{};
}

std::string_view animation_name(CursorKind kind) noexcept {
    switch (kind) {
    case CursorKind::arrow: return "sample_cursor";
    case CursorKind::combat_cast_spell: return "cursor.combat.cast_spell";
    default: return {};
    }
}

std::string direction_group_name(CursorKind kind, Direction16 d) {
    if (!is_direction_kind(kind))
        return {};
    std::string name{group_name(kind)};
    name += '.';
    name += to_string(d);
    return name;
}

CursorState map_edge_cursor(const core::Rect& viewport, core::Point p, std::int32_t margin) noexcept {
    if (!viewport.contains(p))
        return {};
    const bool north = p.y - viewport.top() < margin;
    const bool south = viewport.bottom() - 1 - p.y < margin;
    const bool west  = p.x - viewport.left() < margin;
    const bool east  = viewport.right() - 1 - p.x < margin;
    if (!north && !south && !west && !east)
        return {.kind = CursorKind::move};

    Direction8 d = Direction8::west;
    if (north && east)
        d = Direction8::northeast;
    else if (north && west)
        d = Direction8::northwest;
    else if (south && east)
        d = Direction8::southeast;
    else if (south && west)
        d = Direction8::southwest;
    else if (north)
        d = Direction8::north;
    else if (south)
        d = Direction8::south;
    else if (east)
        d = Direction8::east;
    return {.kind = CursorKind::scroll, .dir8 = d};
}

}
