#pragma once

#include "h4/ai/player_controller.hpp"

namespace h4::ai {

class PassiveController final : public PlayerController {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "passive";
    }
    [[nodiscard]] rules::Command decide(const rules::GameState& state, rules::PlayerId player) override;
};

}
