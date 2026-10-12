#include "h4/app/adventure/adventure_cursor.hpp"

#include <algorithm>

#include "h4/formats/adv_object.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/model/map.hpp"
#include "h4/model/shroud.hpp"
#include "h4/rules/adventure_step.hpp"
#include "h4/scene/map_lattice.hpp"

namespace h4::app {

namespace {

using cursor::CursorKind;

[[nodiscard]] core::Size footprint(const CursorWorld& world, std::uint32_t actor) {
    const formats::AdvObject* model = world.board.model(actor);
    return model != nullptr ? model->size() : core::Size{1, 1};
}

[[nodiscard]] std::optional<rules::MapPoint> bridge_pick(const CursorWorld& world, const scene::MapHover& hover) {
    const std::optional<std::uint32_t> under = object_under(world, hover);
    if (!under.has_value())
        return std::nullopt;
    const formats::AdvObject* model = world.board.model(*under);
    if (model == nullptr || !model->is_bridge())
        return std::nullopt;
    const model::MapActor& bridge = world.map.actors()[*under];
    rules::MapPoint at{bridge.position.x, bridge.position.y, bridge.level, true};
    const core::Size size = model->size();
    if (size.width > 1 || size.height > 1) {
        const rules::MapPoint next = size.width < 2 ? rules::MapPoint{at.x + 1, at.y, at.level, true} :
                                                      rules::MapPoint{at.x, at.y + 1, at.level, true};
        const std::int32_t vertex = scene::box_pin({next.x, next.y}, world.map.dimension()).x;
        const bool left           = hover.point.x < vertex;
        if (size.width < 2 ? left : !left)
            at = next;
    }
    return at;
}

[[nodiscard]] CursorKind object_offer(const CursorWorld& world, std::uint32_t mover, std::uint32_t target) {
    const model::MapActor& self  = world.map.actors()[mover];
    const model::MapActor& other = world.map.actors()[target];
    const bool friendly          = world.friendly(self.owner, other.owner);
    switch (other.kind) {
    case model::ActorKind::town: return friendly ? CursorKind::activate : CursorKind::attack;
    case model::ActorKind::garrison: return friendly || other.army.empty() ? CursorKind::activate : CursorKind::attack;
    default: break;
    }
    if (world.board.is_army(target)) {
        if (friendly)
            return CursorKind::exchange;
        return other.army.empty() ? CursorKind::activate : CursorKind::attack;
    }
    return CursorKind::activate;
}

[[nodiscard]] rules::OwnsStep not_itself(const MapMover& mover) {
    return [actor = mover.actor](std::uint32_t owner) { return owner != actor; };
}

[[nodiscard]] std::optional<MoveTarget> reach(const CursorWorld& world, const MapMover& mover, rules::MapPoint at) {
    if (mover.zones != nullptr)
        if (const rules::ZoneCell zone = mover.zones->at(at); zone.marked && world.board.objects(at).empty()) {
            const auto owner = rules::step_owner(world.board, at, zone.dir, not_itself(mover));
            if (owner.has_value() && world.board.is_army(*owner)) {
                const core::Point delta = rules::kStepDeltas[static_cast<std::size_t>(zone.dir)];
                at.x += delta.x;
                at.y += delta.y;
            }
        }
    const std::optional<std::int32_t> total = mover.field->total(at);
    if (!total.has_value())
        return std::nullopt;
    return MoveTarget{at, *total};
}

}

rules::PathField::Rules land_search_rules(const rules::AdventureBoard& board, std::uint32_t mover, const model::Shroud* seen) {
    rules::PathField::Rules out;
    out.blocks = [&board, mover, seen](std::uint32_t actor) {
        if (actor == mover || !board.is_army(actor))
            return false;
        const rules::MapPoint at = board.origin(actor);
        return seen == nullptr || seen->level(at.x, at.y, at.level) == model::kFogVisible;
    };
    if (seen != nullptr)
        out.hidden = [seen](const rules::MapPoint& at) { return seen->level(at.x, at.y, at.level) == model::kFogHidden; };
    return out;
}

std::optional<std::uint32_t> object_under(const CursorWorld& world, const scene::MapHover& hover) {
    if (hover.sprite.has_value())
        return hover.sprite;
    if (!hover.tile.has_value() || !world.explored(*hover.tile))
        return std::nullopt;
    const auto listed = world.board.objects({hover.tile->x, hover.tile->y, hover.level, false});
    if (listed.empty())
        return std::nullopt;
    return listed.front();
}

std::optional<MoveTarget> move_target(const CursorWorld& world, const MapMover& mover, const scene::MapHover& hover) {
    std::optional<rules::MapPoint> first = bridge_pick(world, hover);
    if (!first.has_value() && hover.tile.has_value())
        first = rules::MapPoint{hover.tile->x, hover.tile->y, hover.level, false};
    if (first.has_value())
        if (std::optional<MoveTarget> found = reach(world, mover, *first); found.has_value())
            return found;

    const std::optional<std::uint32_t> under = object_under(world, hover);
    if (!under.has_value())
        return std::nullopt;
    const rules::MapPoint origin = world.board.origin(*under);
    const core::Size box         = footprint(world, *under);
    std::optional<MoveTarget> best;
    for (std::int32_t i = 0; i < box.width; ++i)
        for (std::int32_t j = 0; j < box.height; ++j) {
            const rules::MapPoint cell{origin.x + i, origin.y + j, origin.level, false};
            const auto listed = world.board.objects(cell);
            if (std::ranges::find(listed, *under) == listed.end())
                continue;
            const std::optional<MoveTarget> found = reach(world, mover, cell);
            if (found.has_value() && (!best.has_value() || found->total < best->total))
                best = found;
        }
    return best;
}

cursor::CursorState map_cursor(const CursorWorld& world, const std::optional<MapMover>& mover, const scene::MapHover& hover) {
    cursor::CursorState state;
    if (!mover.has_value() || mover->field == nullptr || mover->day <= 0)
        return state;
    const std::optional<MoveTarget> target = move_target(world, *mover, hover);
    if (!target.has_value())
        return state;

    state.kind = CursorKind::move;
    std::optional<std::uint32_t> owner;
    if (const auto last = mover->field->last_step(target->to); last.has_value())
        owner = rules::step_owner(world.board, last->from, last->dir, not_itself(*mover));
    if (owner.has_value() && world.explored({target->to.x, target->to.y}))
        state.kind = object_offer(world, mover->actor, *owner);
    else if (mover->zones != nullptr && mover->zones->at(target->to).marked)
        state.kind = CursorKind::attack;

    const std::int32_t over = target->total - mover->left;
    const std::int32_t days = over > 0 ? (over + mover->day - 1) / mover->day : 0;
    state.days              = std::min(days, 3) + 1;
    return state;
}

}
