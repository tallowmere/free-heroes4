#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/game_settings.hpp"

namespace h4::model {

inline constexpr std::string_view kSettingsFile = "settings.ini";

struct UserSettings {
    std::filesystem::path data_dir;
    GameSettings game;

    [[nodiscard]] std::string serialize() const;

    friend bool operator==(const UserSettings&, const UserSettings&) = default;
};

struct LoadedSettings {
    UserSettings settings;
    std::vector<std::string> warnings;
    bool found = false;
};

[[nodiscard]] LoadedSettings parse_user_settings(std::string_view text);

[[nodiscard]] core::Result<LoadedSettings> load_user_settings(const std::filesystem::path& path);

[[nodiscard]] core::Result<void> save_user_settings(const std::filesystem::path& path, const UserSettings& settings);

}
