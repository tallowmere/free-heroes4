#pragma once

#include <string_view>

#include "h4/rules/command.hpp"
#include "h4/rules/game_state.hpp"

namespace h4::ai {

class PlayerController {
public:
    virtual ~PlayerController() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    [[nodiscard]] virtual rules::Command decide(const rules::GameState& state, rules::PlayerId player) = 0;

protected:
    PlayerController()                                   = default;
    PlayerController(const PlayerController&)            = default;
    PlayerController& operator=(const PlayerController&) = default;
    PlayerController(PlayerController&&)                 = default;
    PlayerController& operator=(PlayerController&&)      = default;
};

}
