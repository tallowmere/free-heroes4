#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "h4/core/geometry.hpp"
#include "h4/cursor/direction.hpp"

namespace h4::cursor {

enum class CursorKind : std::uint8_t {
    arrow,
    move,
    activate,
    attack,
    exchange,
    anchor,
    boat_move,
    boat_activate,
    danger_zone,
    scuttle_boat,
    blocked,
    wait,
    split,
    scroll,
    combat_normal,
    combat_walk,
    combat_fly,
    combat_shoot,
    combat_melee,
    combat_melee_dir,
    combat_arrow_dir,
    combat_info,
    combat_cast_spell,
    combat_no_cast,
    combat_tower,
    combat_displacement,
    combat_teleport,
    combat_quicksand,
    combat_martyr,
    combat_martyr_client,
    combat_sacrifice,
    combat_sacrifice_client,
    combat_attack_gate,
};

inline constexpr std::size_t kCursorKindCount = std::to_underlying(CursorKind::combat_attack_gate) + 1;

struct CursorState {
    CursorKind kind   = CursorKind::arrow;
    std::int32_t days = 1;
    Direction8 dir8   = Direction8::north;
    Direction16 dir16 = Direction16::north;

    friend constexpr bool operator==(const CursorState&, const CursorState&) noexcept = default;
};

[[nodiscard]] std::string_view group_name(CursorKind kind) noexcept;

[[nodiscard]] std::string_view animation_name(CursorKind kind) noexcept;

[[nodiscard]] std::string direction_group_name(CursorKind kind, Direction16 d);
[[nodiscard]] constexpr bool is_direction_kind(CursorKind kind) noexcept {
    return kind == CursorKind::combat_melee_dir || kind == CursorKind::combat_arrow_dir;
}

inline constexpr std::int32_t kMapScrollMargin = 8;
[[nodiscard]] CursorState
map_edge_cursor(const core::Rect& viewport, core::Point pointer, std::int32_t margin = kMapScrollMargin) noexcept;

}
