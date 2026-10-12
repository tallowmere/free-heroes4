#include "h4/app/boot/new_game_menu.hpp"

namespace h4::app {

std::vector<NewGameEntry> new_game_entries(const model::InstalledContent& content) {
    using model::Expansion;
    std::vector<NewGameEntry> entries{{NewGameChoice::scenarios, "main_menu_scenario.misc"}};
    if (!content.has_any_expansion()) {
        entries.push_back({NewGameChoice::campaigns, "main_menu_campaign.misc"});
    } else {
        if (content.has(Expansion::winds_of_war))
            entries.push_back({NewGameChoice::winds_of_war_campaigns, "new_campaign_x2"});
        if (content.has(Expansion::gathering_storm))
            entries.push_back({NewGameChoice::gathering_storm_campaigns, "storm_new_campaign_storm"});
        entries.push_back({NewGameChoice::heroes_iv_campaigns, "storm_new_campaign_original"});
    }
    entries.push_back({NewGameChoice::tutorial, "main_menu_tutorial.misc"});
    return entries;
}

std::string_view display_name(NewGameChoice choice) noexcept {
    switch (choice) {
    case NewGameChoice::scenarios: return "Scenarios";
    case NewGameChoice::campaigns: return "Campaigns";
    case NewGameChoice::winds_of_war_campaigns: return "Winds of War Campaigns";
    case NewGameChoice::gathering_storm_campaigns: return "Gathering Storm Campaigns";
    case NewGameChoice::heroes_iv_campaigns: return "Heroes IV Campaigns";
    case NewGameChoice::tutorial: return "Tutorial";
    }
    return "?";
}

}
