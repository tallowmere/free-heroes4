#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/model/player_colour.hpp"

namespace h4::model {

enum class MapSize : std::uint8_t { small, medium, large, extra_large };

enum class MapDifficulty : std::uint8_t { easy, normal, hard, expert, impossible };

[[nodiscard]] constexpr std::string_view to_string(MapDifficulty difficulty) noexcept {
    constexpr std::array<std::string_view, 5> kWords{"easy", "normal", "hard", "expert", "impossible"};
    return kWords[std::to_underlying(difficulty)];
}

enum class MapEdition : std::uint8_t { original, gathering_storm, winds_of_war };

[[nodiscard]] MapSize map_size_of(std::uint16_t grid_dimension) noexcept;

struct ScenarioSlot {
    PlayerColour colour     = PlayerColour::red;
    bool human_capable      = false;
    std::uint8_t alignments = 0;
    std::optional<std::size_t> team;
};

struct ScenarioSummary {
    std::filesystem::path path;
    std::string name;
    std::string description;
    MapSize size             = MapSize::small;
    MapDifficulty difficulty = MapDifficulty::easy;
    std::size_t players      = 0;
    std::size_t humans       = 0;
    std::size_t alliances    = 0;
    std::optional<MapEdition> edition;
    bool multiplayer = false;
    std::vector<ScenarioSlot> slots;
    std::optional<std::string> loss_text;
    std::optional<std::string> victory_text;
};

}
