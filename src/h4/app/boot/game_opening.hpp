#pragma once

#include <optional>

#include "h4/core/error.hpp"
#include "h4/model/scenario.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/rules/game_state.hpp"

namespace h4::model {
class GameDatabase;
}

namespace h4::app {

struct GameOpening {
    model::Scenario scenario;
    model::ScenarioSetup setup;
    rules::GameState game;
    std::optional<rules::PlayerId> me;
};

[[nodiscard]] core::Result<GameOpening>
open_game(model::Scenario scenario, model::ScenarioSetup setup, const model::GameDatabase* database);

[[nodiscard]] core::Result<GameOpening>
resolve_and_open(model::Scenario scenario, model::ScenarioSetup setup, const model::GameDatabase* database);

}
