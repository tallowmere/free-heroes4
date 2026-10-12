#pragma once

#include "h4/model/scenario.hpp"

namespace h4::model {

class GameDatabase;
class ScenarioSetup;

[[nodiscard]] Scenario resolved_scenario(Scenario scenario, const ScenarioSetup& setup, const GameDatabase* database);

}
