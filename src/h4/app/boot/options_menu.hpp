#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "h4/model/installation.hpp"

namespace h4::app {

enum class OptionsChoice : std::uint8_t {
    game_settings,
    high_scores,
    replay_cinematic,
    replay_storm_cinematic,
    replay_winds_of_war_cinematic,
    credits,
};

struct OptionsEntry {
    OptionsChoice choice;
    std::string_view text_key;

    friend constexpr bool operator==(const OptionsEntry&, const OptionsEntry&) noexcept = default;
};

[[nodiscard]] std::vector<OptionsEntry> options_entries(const model::InstalledContent& content);

[[nodiscard]] std::string_view display_name(OptionsChoice choice) noexcept;

}
