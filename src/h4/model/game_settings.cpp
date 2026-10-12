#include "h4/model/game_settings.hpp"

#include <array>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/core/plain_text.hpp"
#include "h4/io/mapped_file.hpp"

namespace h4::model {

GameSettings GameSettings::parse_ini(std::string_view text) {
    GameSettings settings;
    const std::array<std::pair<std::string_view, std::int32_t*>, 6> numbers{{
        {"resolution", &settings.resolution},
        {"army_speed", &settings.army_speed},
        {"combat_speed", &settings.combat_speed},
        {"enemy_speed", &settings.enemy_speed},
        {"music_volume", &settings.music_volume},
        {"sound_volume", &settings.sound_volume},
    }};
    const std::array<std::pair<std::string_view, bool*>, 8> flags{{
        {"full_screen", &settings.full_screen},
        {"quick_combat", &settings.quick_combat},
        {"show_coordinates", &settings.show_coordinates},
        {"army_movement_reminder", &settings.army_movement_reminder},
        {"show_move_path", &settings.show_move_path},
        {"adventure_animations", &settings.adventure_animations},
        {"guards_move", &settings.guards_move},
        {"follow_window", &settings.follow_window},
    }};

    bool in_application = false;
    while (!text.empty()) {
        const std::string_view line = core::take_line(text);
        if (line.starts_with('[')) {
            in_application = core::iequals(line, "[Application]");
            continue;
        }
        const std::size_t equals = line.find('=');
        if (!in_application || equals == std::string_view::npos)
            continue;
        const std::string_view key = core::trimmed(line.substr(0, equals));
        const auto number          = core::whole_number<std::int32_t>(core::trimmed(line.substr(equals + 1)));
        if (!number)
            continue;
        for (const auto& [name, field] : numbers)
            if (core::iequals(key, name))
                *field = *number;
        for (const auto& [name, field] : flags)
            if (core::iequals(key, name))
                *field = *number != 0;
    }
    return settings;
}

GameSettings GameSettings::load(const std::filesystem::path& ini) {
    const auto file = io::MappedFile::open(ini);
    if (!file)
        return {};
    const auto bytes = file->bytes();
    return parse_ini(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

}
