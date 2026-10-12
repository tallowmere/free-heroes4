#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/core/ascii.hpp"

namespace h4::app {

namespace {

[[nodiscard]] bool names_events(const model::MapActor& actor) noexcept {
    return actor.family == "event_trigger" || actor.family == "pandoras_box";
}

}

bool AdventureController::run_tile_triggers(std::uint32_t army, const rules::MapPoint& at) {
    if (!game_ || !me_ || !moves_)
        return false;
    if (const auto it = triggered_.find(army); it != triggered_.end() && it->second == at)
        return false;
    triggered_[army] = at;

    const auto actors          = scenario_.map().actors();
    const model::SetupRow& row = setup_.rows()[me_->value()];
    const auto& events         = scenario_.placed_events();
    bool spoke                 = false;
    for (const std::uint32_t object : moves_->objects_at(at)) {
        if (object >= actors.size() || !names_events(actors[object]))
            continue;
        const std::string& name = actors[object].message;
        for (std::uint32_t i = 0; i < events.size(); ++i) {
            if (!core::iequals(events[i].name, name) || game_->events()[i].gone || !events[i].admits(row.colour, row.human))
                continue;
            if (!apply(rules::commands::FireEvent{*me_, i}))
                continue;
            for (const std::string& words : events[i].words) {
                if (words.empty())
                    continue;
                messages_.add(words);
                spoke = true;
            }
        }
    }
    if (spoke) {
        show_purse();
        messages_.show();
    }
    return spoke;
}

}
