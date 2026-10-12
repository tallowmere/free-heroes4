#include "h4/app/adventure/system_menu.hpp"

#include <array>

namespace h4::app {

namespace {

struct MenuSpec {
    SystemMenuChoice choice;
    std::string_view text_key;
    std::string_view words;
};

constexpr std::array<MenuSpec, 8> kSystemMenu{{
    {SystemMenuChoice::new_scenario, "new_scenario", "New Scenario"},
    {SystemMenuChoice::new_campaign, "new_campaign", "New Campaign"},
    {SystemMenuChoice::load_game, "load_game", "Load Game"},
    {SystemMenuChoice::save_game, "save_game", "Save Game"},
    {SystemMenuChoice::restart_game, "restart_game", "Restart Game"},
    {SystemMenuChoice::game_settings, "options", "Game Settings"},
    {SystemMenuChoice::main_menu, "main_menu", "Main Menu"},
    {SystemMenuChoice::quit, "quit", "Quit"},
}};

}

std::vector<SystemMenuEntry> system_menu_entries() {
    std::vector<SystemMenuEntry> out;
    for (const MenuSpec& spec : kSystemMenu)
        out.push_back({spec.choice, spec.text_key});
    return out;
}

std::string_view display_name(SystemMenuChoice choice) noexcept {
    for (const MenuSpec& spec : kSystemMenu)
        if (spec.choice == choice)
            return spec.words;
    return "?";
}

}
