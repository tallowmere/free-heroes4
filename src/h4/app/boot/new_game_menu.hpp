#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "h4/model/installation.hpp"

namespace h4::app {

enum class NewGameChoice : std::uint8_t {
    scenarios,
    campaigns,
    winds_of_war_campaigns,
    gathering_storm_campaigns,
    heroes_iv_campaigns,
    tutorial,
};

struct NewGameEntry {
    NewGameChoice choice;
    std::string_view text_key;

    friend constexpr bool operator==(const NewGameEntry&, const NewGameEntry&) noexcept = default;
};

[[nodiscard]] std::vector<NewGameEntry> new_game_entries(const model::InstalledContent& content);

[[nodiscard]] std::string_view display_name(NewGameChoice choice) noexcept;

}
