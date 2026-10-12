#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace h4::model {

struct GameSettings {
    std::int32_t resolution     = 1024;
    bool full_screen            = false;
    bool quick_combat           = false;
    bool show_coordinates       = true;
    bool army_movement_reminder = true;
    bool show_move_path         = true;
    bool adventure_animations   = true;
    std::int32_t army_speed     = 150;
    std::int32_t combat_speed   = 150;
    std::int32_t enemy_speed    = 200;
    std::int32_t music_volume   = 100;
    std::int32_t sound_volume   = 100;
    bool guards_move = true;
    bool follow_window = true;

    [[nodiscard]] static GameSettings parse_ini(std::string_view text);
    [[nodiscard]] static GameSettings load(const std::filesystem::path& ini);

    friend bool operator==(const GameSettings&, const GameSettings&) = default;
};

inline constexpr std::int32_t kSpeedStep  = 50;
inline constexpr std::int32_t kSpeedSteps = 8;
inline constexpr std::int32_t kVolumeMax = 100;

}
