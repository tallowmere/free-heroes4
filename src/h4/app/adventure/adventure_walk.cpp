#include <limits>
#include <vector>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/assets/assets.hpp"
#include "h4/rules/army_movement.hpp"

namespace h4::app {

namespace {

constexpr std::int32_t kArmySpeed = 150;

[[nodiscard]] core::Point cell_of(const rules::MapPoint& at) noexcept {
    return {at.x, at.y};
}

}

const rules::ArmyPlace* AdventureController::placement_of(const model::MapActor* army) const noexcept {
    const auto index = scenario_.map().index_of(army);
    return game_.has_value() && index ? game_->army(*index) : nullptr;
}

std::int32_t AdventureController::left_of(const model::MapActor& army) const noexcept {
    const rules::ArmyPlace* place = placement_of(&army);
    return place != nullptr ? place->left : std::numeric_limits<std::int32_t>::max();
}

MovementLeft AdventureController::movement_left() const {
    return [this](const model::MapActor& army) -> std::optional<int> {
        const std::int32_t left = left_of(army);
        if (left == std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        return left;
    };
}

std::int32_t AdventureController::spent_of(const model::MapActor* army) const noexcept {
    const rules::ArmyPlace* place = placement_of(army);
    return place != nullptr ? place->day - place->left : 0;
}

void AdventureController::sync_armies() {
    if (!game_.has_value())
        return;
    const auto actors = scenario_.map().actors();
    for (const rules::ArmyPlace& place : game_->armies()) {
        const model::MapActor& actor = actors[place.actor];
        if (place.at == rules::MapPoint{actor.position.x, actor.position.y, actor.level, false})
            continue;
        if (moves_)
            moves_->army_moved(place.actor, place.at);
        scene::ArmyPose pose;
        if (const auto was = view_->map_window().army_pose(place.actor); was.has_value())
            pose.facing = was->facing;
        pose.bridge = place.at.bridge;
        place_army(place.actor, place.at, pose);
    }
}

void AdventureController::place_army(std::uint32_t actor, const rules::MapPoint& at, const scene::ArmyPose& pose) {
    view_->map_window().pose_army(actor, cell_of(at), at.level, pose);
    if (shroud_)
        shroud_->moved(actor, cell_of(at), at.level);
}

void AdventureController::start_walk() {
    if (journey_.walking() || !moves_ || !game_ || !me_ || selected_ == nullptr || !view_->has_map())
        return;
    const AdventureMoves::Walk walk = moves_->walk();
    const auto actor                = static_cast<std::uint32_t>(selected_ - scenario_.map().actors().data());
    std::optional<Journey::Visit> ends_on;
    if (walk.visit_step.has_value())
        ends_on = Journey::Visit{
            walk.visited, *walk.visit_step, walk.steps.empty() ? moves_->position(actor) : walk.steps.back().at
        };
    if (walk.steps.empty()) {
        journey_.begin(actor, ends_on, nullptr);
        if (const std::optional<Journey::Visit> pending = journey_.visit_from(moves_->position(actor)))
            visit(pending->object, pending->step);
        return;
    }
    std::optional<scene::ArmyFigure> figure = view_->map_window().army_figure(actor);
    if (!figure)
        return;
    const auto facing = view_->map_window().army_pose(actor);
    journey_.begin(
        actor,
        ends_on,
        std::make_unique<ArmyMover>(
            moves_->position(actor), facing.has_value() ? facing->facing : 0, walk.steps, std::move(*figure), view_->clock_ms(), kArmySpeed
        )
    );
}

void AdventureController::on_ticked(std::uint32_t now) {
    if (!journey_.walking())
        return;
    journey_.tick(
        now,
        messages_.is_open(),
        [this](const ArmyMover::Arrival& arrival) { return on_arrived(arrival); },
        [this] {
            (void)run_tile_triggers(journey_.actor(), journey_.mover().point());
            return true;
        }
    );
    if (!journey_.walking())
        return;
    place_army(journey_.actor(), journey_.mover().point(), journey_.mover().pose());
    if (journey_.mover().finished())
        end_walk();
}

bool AdventureController::on_arrived(const ArmyMover::Arrival& arrival) {
    if (!game_ || !me_)
        return false;
    if (!apply(rules::commands::MoveArmy{*me_, journey_.actor(), arrival.at, journey_.uncharged(arrival.total)}))
        return false;
    journey_.charged_to(arrival.total);
    if (moves_)
        moves_->army_moved(journey_.actor(), arrival.at);
    if (!path_.empty()) {
        path_.erase(path_.begin());
        present_map();
    }
    view_->map_window().keep_in_sight(cell_of(arrival.at), arrival.at.level, arrival.at.bridge);
    return true;
}

void AdventureController::end_walk() {
    const std::optional<Journey::Rest> rest = journey_.end();
    if (!rest)
        return;
    const rules::MapPoint& at = rest->at;
    place_army(journey_.actor(), at, rest->pose);
    (void)run_tile_triggers(journey_.actor(), at);
    if (const std::optional<Journey::Visit> pending = journey_.visit_from(at)) {
        visit(pending->object, pending->step);
        return;
    }
    const std::optional<rules::MapPoint> destination = moves_ ? moves_->destination() : std::nullopt;
    select(selected_);
    if (moves_ && destination.has_value() && !(*destination == at)) {
        path_ = moves_->keep_path(*destination);
        present_map();
    }
    show_heroes();
}

}
