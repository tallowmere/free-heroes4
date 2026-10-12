#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "h4/app/controller.hpp"
#include "h4/core/signal.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/model/scenario_summary.hpp"
#include "h4/screens/new_game/scenario_options_view.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

class ScenarioOptionsController {
public:
    ScenarioOptionsController(screens::ScenarioOptionsView& view,
                              const assets::Assets& assets,
                              const model::ScenarioSummary& scenario,
                              bool guards_move,
                              Reporter report);

    ScenarioOptionsController(const ScenarioOptionsController&)            = delete;
    ScenarioOptionsController& operator=(const ScenarioOptionsController&) = delete;

    core::Signal<> closed;
    core::Signal<model::ScenarioSetup> began;

    [[nodiscard]] const model::ScenarioSetup& setup() const noexcept {
        return setup_;
    }

    void begin();

    void detach() noexcept;

private:
    void connect_view();
    void apply(model::ScenarioSetup setup);
    [[nodiscard]] screens::ScenarioOptionsModel model() const;

    screens::ScenarioOptionsView* view_;
    const assets::Assets* assets_;
    Reporter report_;
    model::ScenarioSetup setup_;
    std::string conditions_;
    std::string description_;
    core::Connections connections_;
};

}
