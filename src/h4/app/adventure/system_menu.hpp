#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace h4::app {

enum class SystemMenuChoice : std::uint8_t {
    new_scenario,
    new_campaign,
    load_game,
    save_game,
    restart_game,
    game_settings,
    main_menu,
    quit
};

struct SystemMenuEntry {
    SystemMenuChoice choice;
    std::string_view text_key;
};

[[nodiscard]] std::vector<SystemMenuEntry> system_menu_entries();

[[nodiscard]] std::string_view display_name(SystemMenuChoice choice) noexcept;

}
