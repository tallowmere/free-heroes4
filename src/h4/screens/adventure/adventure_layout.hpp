#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "h4/core/ascii.hpp"
#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/screens/kit/button_table.hpp"

namespace h4::formats {
class BitmapGroup;
}

namespace h4::screens {

enum class AdventureButton : std::uint8_t {
    system_menu,
    game_menu,
    surface,
    underground,
    spell,
    move_army,
    marketplace,
    overview,
    end_turn
};

inline constexpr std::array<AdventureButton, 9> kAdventureButtons{AdventureButton::system_menu,
                                                                  AdventureButton::game_menu,
                                                                  AdventureButton::surface,
                                                                  AdventureButton::underground,
                                                                  AdventureButton::spell,
                                                                  AdventureButton::move_army,
                                                                  AdventureButton::marketplace,
                                                                  AdventureButton::overview,
                                                                  AdventureButton::end_turn};

enum class AdventureResource : std::uint8_t { gold, wood, ore, mercury, sulfur, crystal, gems };

inline constexpr std::size_t kAdventureResources = 7;

[[nodiscard]] const ButtonSpec& button_spec(AdventureButton button) noexcept;

[[nodiscard]] bool spans_map_window(std::string_view layer) noexcept;
[[nodiscard]] bool stands_left_of_map(std::string_view layer) noexcept;

struct AdventureLayout {
    [[nodiscard]] static core::Result<AdventureLayout> from_group(const formats::BitmapGroup& group, std::int32_t frame_width = 0);

    core::Rect viewport;
    core::Rect mini_map;
    core::Rect hero_list;
    core::Rect hero_scrollbar;
    core::Rect town_list;
    core::Rect town_scrollbar;
    core::Rect army;
    core::Rect day_scroll;
    std::int32_t rail_cut = 0;
    core::Rect creature_name;
    core::Rect hero_class;
    core::Rect creature_health;
    core::Rect creature_damage;
    core::Rect creature_move;
    core::Rect creature_mana;
    std::array<core::Rect, 8> creature_skills{};
    core::Rect luck;
    core::Rect morale;

    std::array<core::Rect, 9> buttons{};
    std::array<core::Rect, kAdventureResources> resource_numbers{};
    core::Size size;

    std::int32_t panel_shift = 0;
    core::Rect top_rail;
    core::Rect bottom_rail;
    core::Rect top_border;
    core::Rect bottom_border;
};

}
