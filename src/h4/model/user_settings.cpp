#include "h4/model/user_settings.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <variant>

#include "h4/core/ascii.hpp"
#include "h4/core/plain_text.hpp"
#include "h4/io/file_read.hpp"
#include "h4/io/file_write.hpp"

namespace h4::model {

namespace {

constexpr std::int32_t kVersion = 1;

struct Field {
    std::string_view section;
    std::string_view key;
    std::variant<std::int32_t GameSettings::*, bool GameSettings::*> member;
};

constexpr std::array<Field, 14> kFields{{
    {"game", "resolution", &GameSettings::resolution},
    {"game", "full_screen", &GameSettings::full_screen},
    {"game", "quick_combat", &GameSettings::quick_combat},
    {"game", "show_coordinates", &GameSettings::show_coordinates},
    {"game", "army_movement_reminder", &GameSettings::army_movement_reminder},
    {"game", "show_move_path", &GameSettings::show_move_path},
    {"game", "adventure_animations", &GameSettings::adventure_animations},
    {"game", "army_speed", &GameSettings::army_speed},
    {"game", "combat_speed", &GameSettings::combat_speed},
    {"game", "enemy_speed", &GameSettings::enemy_speed},
    {"game", "music_volume", &GameSettings::music_volume},
    {"game", "sound_volume", &GameSettings::sound_volume},
    {"game", "guards_move", &GameSettings::guards_move},
    {"window", "follow_window", &GameSettings::follow_window},
}};

[[nodiscard]] std::optional<bool> flag_of(std::string_view value) noexcept {
    if (core::iequals(value, "true") || value == "1")
        return true;
    if (core::iequals(value, "false") || value == "0")
        return false;
    return std::nullopt;
}

[[nodiscard]] bool assign(const Field& field, GameSettings& game, std::string_view value) {
    if (const auto* number = std::get_if<std::int32_t GameSettings::*>(&field.member)) {
        const auto read = core::whole_number<std::int32_t>(value);
        if (read)
            game.** number = *read;
        return read.has_value();
    }
    const auto read = flag_of(value);
    if (read)
        game.*std::get<bool GameSettings::*>(field.member) = *read;
    return read.has_value();
}

void take(LoadedSettings& out, std::string_view section, std::string_view key, std::string_view value) {
    const auto warn = [&](std::string_view why) {
        out.warnings.push_back(std::format("settings.ini: [{}] {}: {}", section, key, why));
    };
    if (core::iequals(section, "install") && core::iequals(key, "version")) {
        if (core::whole_number<std::int32_t>(value) != kVersion)
            warn(std::format("version '{}', only {} is known", value, kVersion));
        return;
    }
    if (core::iequals(section, "install") && core::iequals(key, "data_dir")) {
        out.settings.data_dir = std::filesystem::path{std::u8string{value.begin(), value.end()}};
        return;
    }
    for (const Field& field : kFields) {
        if (!core::iequals(section, field.section) || !core::iequals(key, field.key))
            continue;
        if (!assign(field, out.settings.game, value))
            warn(std::format("'{}' does not read", value));
        return;
    }
    warn("unknown key");
}

}

std::string UserSettings::serialize() const {
    const auto path          = data_dir.u8string();
    std::string out          = std::format("[install]\nversion = {}\ndata_dir = {}\n",
                                           kVersion,
                                           std::string_view{reinterpret_cast<const char*>(path.data()), path.size()});
    std::string_view section = "install";
    for (const Field& field : kFields) {
        if (field.section != section) {
            section = field.section;
            out += std::format("\n[{}]\n", section);
        }
        if (const auto* number = std::get_if<std::int32_t GameSettings::*>(&field.member))
            out += std::format("{} = {}\n", field.key, game.**number);
        else
            out += std::format("{} = {}\n", field.key, game.*std::get<bool GameSettings::*>(field.member));
    }
    return out;
}

LoadedSettings parse_user_settings(std::string_view text) {
    LoadedSettings out;
    out.found = true;
    std::string_view section;
    while (!text.empty()) {
        const std::string_view line = core::take_line(text);
        if (line.empty() || line.front() == '#' || line.front() == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = core::trimmed(line.substr(1, line.size() - 2));
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string_view::npos) {
            out.warnings.push_back(std::format("settings.ini: '{}' is not key = value", line));
            continue;
        }
        take(out, section, core::trimmed(line.substr(0, equals)), core::trimmed(line.substr(equals + 1)));
    }
    return out;
}

core::Result<LoadedSettings> load_user_settings(const std::filesystem::path& path) {
    const auto text = io::read_text_file(path);
    if (!text) {
        if (text.error().code() == core::ErrorCode::not_found)
            return LoadedSettings{};
        return std::unexpected(text.error());
    }
    return parse_user_settings(*text);
}

core::Result<void> save_user_settings(const std::filesystem::path& path, const UserSettings& settings) {
    return io::write_file_atomically(path, settings.serialize());
}

}
