#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "h4/core/clock.hpp"

namespace h4::formats {
enum class CombatState : std::uint8_t;
}

namespace h4::model {
class Roller;
}

namespace h4::screens {

class CreatureIdle {
public:
    static constexpr std::size_t kStates = 16;
    struct Model {
        std::array<std::size_t, kStates> frames{};
        std::array<std::uint32_t, kStates> rates{};
    };

    explicit CreatureIdle(Model model);

    bool advance(core::Instant now, model::Roller& seed);

    [[nodiscard]] formats::CombatState state() const noexcept {
        return state_;
    }
    [[nodiscard]] std::size_t frame() const noexcept {
        return frame_;
    }
    [[nodiscard]] std::chrono::milliseconds interval() const noexcept {
        return timer_.interval();
    }

private:
    [[nodiscard]] bool has(formats::CombatState state) const noexcept;
    [[nodiscard]] std::size_t frames(formats::CombatState state) const noexcept;
    [[nodiscard]] std::uint32_t rate(formats::CombatState state) const noexcept;

    void enter(formats::CombatState asked);
    void tick(model::Roller& seed);

    Model model_;
    formats::CombatState state_{};
    formats::CombatState asked_{};
    std::size_t frame_   = 0;
    std::uint32_t loops_ = 1;
    core::Timer timer_;
};

}
