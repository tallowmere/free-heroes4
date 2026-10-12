#include "h4/ai/passive_controller.hpp"

namespace h4::ai {

rules::Command PassiveController::decide(const rules::GameState& , rules::PlayerId player) {
    return rules::commands::EndTurn{player};
}

}
