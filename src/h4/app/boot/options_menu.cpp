#include "h4/app/boot/options_menu.hpp"

namespace h4::app {

std::vector<OptionsEntry> options_entries(const model::InstalledContent& content) {
    using model::Expansion;
    std::vector<OptionsEntry> entries{
        {OptionsChoice::game_settings, "main_menu_settings.misc"},
        {OptionsChoice::high_scores, "main_menu_high_score.misc"},
        {OptionsChoice::replay_cinematic, "main_menu_replay_intro.misc"},
    };
    if (content.has(Expansion::gathering_storm))
        entries.push_back({OptionsChoice::replay_storm_cinematic, "main_menu_replay_intro.storm_misc"});
    if (content.has(Expansion::winds_of_war))
        entries.push_back({OptionsChoice::replay_winds_of_war_cinematic, "main_menu_replay_intro.x2_misc"});
    entries.push_back({OptionsChoice::credits, "main_menu_credits.misc"});
    return entries;
}

std::string_view display_name(OptionsChoice choice) noexcept {
    switch (choice) {
    case OptionsChoice::game_settings: return "Game Settings";
    case OptionsChoice::high_scores: return "High Scores";
    case OptionsChoice::replay_cinematic: return "Replay Cinematic";
    case OptionsChoice::replay_storm_cinematic: return "Replay Storm Cinematic";
    case OptionsChoice::replay_winds_of_war_cinematic: return "Replay Winds of War Cinematic";
    case OptionsChoice::credits: return "Credits";
    }
    return "?";
}

}
