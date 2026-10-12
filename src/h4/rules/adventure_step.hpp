#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

#include "h4/core/geometry.hpp"
#include "h4/rules/adventure_board.hpp"

namespace h4::rules {

inline constexpr std::array<core::Point, 8> kStepDeltas{{{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}};
inline constexpr int kDirections = 8;

[[nodiscard]] constexpr bool is_diagonal(int dir) noexcept {
    return (dir & 1) != 0;
}

[[nodiscard]] bool step_changes_layer(const AdventureBoard& board, const MapPoint& from, int dir);

[[nodiscard]] MapPoint step_to(const AdventureBoard& board, const MapPoint& from, int dir);

[[nodiscard]] bool step_walled(const AdventureBoard& board, const MapPoint& from, int dir);

[[nodiscard]] bool diagonal_corners_walled(const AdventureBoard& board, const MapPoint& from, const MapPoint& to, int dir);

using OwnsStep = std::function<bool(std::uint32_t actor)>;

[[nodiscard]] inline bool owns_every_step(std::uint32_t) noexcept {
    return true;
}

[[nodiscard]] std::optional<std::uint32_t>
step_owner(const AdventureBoard& board, const MapPoint& from, int dir, const OwnsStep& owns);

using TerrainCosts = std::array<std::int32_t, 19>;

[[nodiscard]] std::int32_t
step_cost(const AdventureBoard& board, const MapPoint& from, const MapPoint& to, int dir, const TerrainCosts& terrain);

}
