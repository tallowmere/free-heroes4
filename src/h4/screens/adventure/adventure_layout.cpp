#include "h4/screens/adventure/adventure_layout.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>
#include <utility>

#include "h4/formats/bitmap_group.hpp"

namespace h4::screens {

namespace {

constexpr ButtonSpec frame_button(std::string_view guide, std::string_view group, std::string_view help) {
    return {.guide = guide, .help_item = help, .art = ButtonArt::faces, .faces = group};
}

constexpr std::array<ButtonSpec, 9> kButtons{{
    frame_button("System_menu_button", "button.system_menu", "system_menu"),
    frame_button("Game_menu_button", "button.game_menu", "game_menu"),
    frame_button("surface_button", "button.surface", "surface"),
    frame_button("underground_button", "button.underground", "underground"),
    frame_button("spell_button", "button.spell", "cast_spell"),
    frame_button("move_army_button", "button.move_army", "move_army"),
    frame_button("marketplace_button", "button.marketplace", "marketplace"),
    frame_button("overview_button", "button.overview", "kingdom_overview"),
    frame_button("end_turn", "button.end_turn", "end_turn"),
}};

constexpr std::array<std::string_view, kAdventureResources> kResourceNumbers{"Gold_Number",
                                                                             "Wood_Number",
                                                                             "Ore_Number",
                                                                             "Mercury_Number",
                                                                             "Sulfur_Number",
                                                                             "Crystal_Number",
                                                                             "Gems_Number"};

core::Rect rect_of(const formats::BitmapGroup& group, std::string_view name) {
    const formats::Layer* layer = group.find(name);
    return layer != nullptr ? layer->bounds() : core::Rect{};
}

}

const ButtonSpec& button_spec(AdventureButton button) noexcept {
    return kButtons[std::to_underlying(button)];
}

bool spans_map_window(std::string_view layer) noexcept {
    for (const std::string_view rail : {"top", "bottom", "top_border", "bottom_border"})
        if (core::iequals(layer, rail))
            return true;
    return false;
}

bool stands_left_of_map(std::string_view layer) noexcept {
    return core::iequals(layer, "left") || core::iequals(layer, "left_border");
}

core::Result<AdventureLayout> AdventureLayout::from_group(const formats::BitmapGroup& group, std::int32_t frame_width) {
    AdventureLayout out;
    const core::Rect left   = rect_of(group, "left");
    const core::Rect top    = rect_of(group, "top");
    const core::Rect right  = rect_of(group, "right");
    const core::Rect bottom = rect_of(group, "bottom");
    if (left.empty() || top.empty() || right.empty() || bottom.empty())
        return core::fail(core::ErrorCode::corrupt, "adventure: a chrome strip is missing");

    const core::Size authored = group.bounds().size();
    out.panel_shift           = std::max(0, frame_width - authored.width);
    out.size                  = {authored.width + out.panel_shift, authored.height};

    const auto guide = [&](std::string_view name) {
        const core::Rect rect = rect_of(group, name);
        return rect.empty() ? rect : rect.translated(core::Point{out.panel_shift, 0});
    };

    out.viewport = core::Rect::from_edges(left.right(), top.bottom(), right.left() + out.panel_shift, bottom.top());
    if (out.viewport.empty())
        return core::fail(core::ErrorCode::corrupt, "adventure: the map window is empty");

    const auto spanned = [&out](core::Rect rect) {
        return rect.empty() ? rect : core::Rect{rect.origin(), {rect.width() + out.panel_shift, rect.height()}};
    };
    out.top_rail                  = spanned(top);
    out.bottom_rail               = spanned(bottom);
    const core::Rect right_border = rect_of(group, "right_border");
    out.rail_cut                  = right_border.empty() ? right.left() : right_border.left();
    out.top_border                = spanned(rect_of(group, "top_border"));
    out.bottom_border             = spanned(rect_of(group, "bottom_border"));

    out.mini_map        = guide("mini_map");
    out.hero_list       = guide("Hero_List");
    out.hero_scrollbar  = guide("Army_Scrollbar");
    out.town_list       = guide("Town_list");
    out.town_scrollbar  = guide("Town_Scrollbar");
    out.army            = guide("army");
    out.day_scroll      = guide("day_scroll");
    out.creature_name   = guide("Creature_Name");
    out.hero_class      = guide("hero_class");
    out.creature_health = guide("Creature_Health");
    out.creature_damage = guide("Creature_Damage");
    out.creature_move   = guide("Creature_Move");
    out.creature_mana   = guide("Creature_Mana");
    for (std::size_t i = 0; i < out.creature_skills.size(); ++i)
        out.creature_skills[i] = guide(std::format("creature_skill_{}", i + 1));
    out.luck   = guide("luck");
    out.morale = guide("morale");
    for (const AdventureButton button : kAdventureButtons)
        out.buttons[std::to_underlying(button)] = guide(button_spec(button).guide);
    for (std::size_t i = 0; i < kResourceNumbers.size(); ++i)
        out.resource_numbers[i] = guide(kResourceNumbers[i]);
    return out;
}

}
