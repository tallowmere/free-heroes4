#include "h4/rules/adventure_step.hpp"

#include "h4/formats/scenario/grid.hpp"

namespace h4::rules {

namespace {

[[nodiscard]] constexpr int turn(int dir, int by) noexcept {
    return (dir + by + kDirections) % kDirections;
}

[[nodiscard]] MapPoint beside(const MapPoint& from, int dir) noexcept {
    const core::Point d = kStepDeltas[static_cast<std::size_t>(dir)];
    return {from.x + d.x, from.y + d.y, from.level, from.bridge};
}

[[nodiscard]] std::size_t layer_of(const MapPoint& at) noexcept {
    return at.bridge ? 1U : 0U;
}

}

bool step_changes_layer(const AdventureBoard& board, const MapPoint& from, int dir) {
    const MapPoint to      = beside(from, dir);
    const BoardTile* here  = board.tile(from.x, from.y, from.level);
    const BoardTile* there = board.tile(to.x, to.y, to.level);
    if (here == nullptr || there == nullptr)
        return false;
    if (from.bridge && here->span == 0)
        return false;
    if ((there->span != 0) == from.bridge)
        return false;
    switch (dir) {
    case 0: return here->ramp_x != 0;
    case 6: return here->ramp_y != 0;
    case 4: return there->ramp_x != 0;
    case 2: return there->ramp_y != 0;
    default: break;
    }
    return step_changes_layer(board, from, turn(dir, 1)) || step_changes_layer(board, from, turn(dir, -1)) ||
        step_changes_layer(board, to, turn(dir, 3)) || step_changes_layer(board, to, turn(dir, -3));
}

MapPoint step_to(const AdventureBoard& board, const MapPoint& from, int dir) {
    MapPoint to = beside(from, dir);
    if (step_changes_layer(board, from, dir))
        to.bridge = !from.bridge;
    return to;
}

bool step_walled(const AdventureBoard& board, const MapPoint& from, int dir) {
    const MapPoint to      = beside(from, dir);
    const BoardTile* here  = board.tile(from.x, from.y, from.level);
    const BoardTile* there = board.tile(to.x, to.y, to.level);
    if (here == nullptr || there == nullptr)
        return true;
    if (is_diagonal(dir))
        return diagonal_corners_walled(board, from, to, dir);
    if (step_changes_layer(board, from, dir))
        return from.bridge ? there->blocked != 0 : there->bridge == 0;
    const std::size_t layer = layer_of(from);
    switch (dir) {
    case 6: return here->wall_y[layer] != 0;
    case 0: return here->wall_x[layer] != 0;
    case 2: return there->wall_y[layer] != 0;
    default: return there->wall_x[layer] != 0;
    }
}

bool diagonal_corners_walled(const AdventureBoard& board, const MapPoint& from, const MapPoint& to, int dir) {
    if (!is_diagonal(dir))
        return false;
    return step_walled(board, from, turn(dir, 1)) || step_walled(board, from, turn(dir, -1)) ||
        step_walled(board, to, turn(dir, 3)) || step_walled(board, to, turn(dir, -3));
}

std::optional<std::uint32_t> step_owner(const AdventureBoard& board, const MapPoint& from, int dir, const OwnsStep& owns) {
    const MapPoint to = step_to(board, from, dir);
    if (board.tile(to.x, to.y, to.level) == nullptr)
        return std::nullopt;

    std::optional<std::uint32_t> owner;
    bool army = false;
    for (const std::uint32_t actor : board.objects(to)) {
        if (!board.owns_steps(actor) || !owns(actor))
            continue;
        const bool is_army = board.is_army(actor);
        if (army && !is_army)
            continue;
        owner = actor;
        army  = is_army;
    }
    if (!owner.has_value())
        return std::nullopt;

    const core::Point d     = kStepDeltas[static_cast<std::size_t>(dir)];
    const std::size_t layer = layer_of(to);
    const auto passage      = [&](std::int32_t x, std::int32_t y, bool along_y) {
        const BoardTile* at = board.tile(x, y, from.level);
        return at != nullptr && (along_y ? at->passage_y[layer] : at->passage_x[layer]) != 0;
    };
    if (d.y > 0 && passage(to.x, to.y, true))
        return owner;
    if (d.x > 0 && passage(to.x, to.y, false))
        return owner;
    if (d.y < 0 && passage(from.x + d.x, from.y, true))
        return owner;
    if (d.x < 0 && passage(from.x, from.y + d.y, false))
        return owner;
    return std::nullopt;
}

std::int32_t step_cost(const AdventureBoard& board, const MapPoint& from, const MapPoint& to, int dir, const TerrainCosts& terrain) {
    std::int32_t step = 75;
    if (!to.bridge) {
        const formats::GridTile* here  = board.grid().tile(from.x, from.y, from.level);
        const formats::GridTile* there = board.grid().tile(to.x, to.y, to.level);
        step = there != nullptr && there->terrain < terrain.size() ? terrain[there->terrain] : 100;
        if (here != nullptr && there != nullptr && here->road != 0 && there->road != 0)
            step = there->road == 2 ? 100 : 75;
    }
    if (is_diagonal(dir))
        step += step / 2;
    return step;
}

}
