#include "h4/model/scenario_resolution.hpp"

#include <cstddef>
#include <utility>

#include "h4/model/game_database.hpp"
#include "h4/model/player_alignment.hpp"
#include "h4/model/random_objects.hpp"
#include "h4/model/roller.hpp"
#include "h4/model/scenario_setup.hpp"

namespace h4::model {

Scenario resolved_scenario(Scenario scenario, const ScenarioSetup& setup, const GameDatabase* database) {
    RandomResolution choices{.difficulty = setup.difficulty()};
    if (database != nullptr) {
        choices.creatures = &database->creatures();
        choices.artifacts = &database->artifacts();
        choices.heroes    = &database->heroes();
    }
    Roller roller{scenario.map().terrain_seed()};
    for (std::size_t i = 0; i < setup.rows().size(); ++i) {
        const SetupRow& row = setup.rows()[i];
        const auto colour   = std::to_underlying(row.colour);
        if (colour >= choices.alignments.size())
            continue;
        choices.alignments[colour] = row.alignment.has_value() ? *row.alignment :
                                                                 drawn_alignment(setup.allowed_alignments(i), roller);
    }
    scenario.resolve_random_objects(choices, roller);
    return scenario;
}

}
