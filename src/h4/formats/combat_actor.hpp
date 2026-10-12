#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::formats {

enum class CombatState : std::uint8_t {
    block,
    cast_spell,
    die,
    fidget,
    flinch,
    melee,
    melee_down,
    melee_up,
    postwalk,
    prewalk,
    walk,
    full_walk,
    ranged,
    base_frame,
    wait,
    wait_and_base,
};
inline constexpr std::size_t kCombatStates = 16;
inline constexpr std::size_t kCombatFacings = 8;

class CombatActor {
public:
    [[nodiscard]] static core::Result<CombatActor> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::uint16_t version() const noexcept {
        return version_;
    }
    [[nodiscard]] std::uint8_t height() const noexcept {
        return height_;
    }
    [[nodiscard]] core::Point anchor() const noexcept {
        return anchor_;
    }
    [[nodiscard]] std::string_view sequence(CombatState state, std::size_t facing) const noexcept;
    [[nodiscard]] std::uint8_t rate(CombatState state) const noexcept;

private:
    struct State {
        std::uint8_t rate = 10;
        std::array<std::string, kCombatFacings> facings{};
    };

    std::uint16_t version_ = 0;
    std::uint8_t height_   = 0;
    core::Point anchor_;
    std::array<State, kCombatStates> states_{};
};

}
