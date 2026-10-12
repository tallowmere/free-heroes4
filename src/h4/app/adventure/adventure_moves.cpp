#include "h4/app/adventure/adventure_moves.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "h4/app/adventure/adventure_cursor.hpp"
#include "h4/assets/assets.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/map.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/model/shroud.hpp"
#include "h4/rules/army_movement.hpp"
#include "h4/rules/army_sight.hpp"

namespace h4::app {

rules::AdventureBoard board_of(const assets::Assets& assets, const model::Map& map) {
    std::vector<const formats::AdvObject*> models;
    models.reserve(map.actors().size());
    for (const model::MapActor& actor : map.actors())
        models.push_back(actor.is_army() ? nullptr : assets.object(actor.model));
    return rules::AdventureBoard::build(map, models, assets.transitions());
}

AdventureMoves::AdventureMoves(const assets::Assets& assets,
                               const model::Map& map,
                               const model::ScenarioSetup& setup,
                               const model::GameDatabase* database,
                               const model::Shroud* seen)
: map_{&map}, database_{database}, seen_{seen}, board_{board_of(assets, map)} {
    for (std::size_t row = 0; row < setup.rows().size(); ++row) {
        const auto colour = std::to_underlying(setup.rows()[row].colour);
        if (colour >= teams_.size())
            continue;
        teams_[colour]  = setup.team(row);
        humans_[colour] = setup.rows()[row].human;
    }
}

bool AdventureMoves::friendly(std::uint8_t a, std::uint8_t b) const noexcept {
    if (a == b)
        return true;
    if (a >= teams_.size() || b >= teams_.size())
        return false;
    return teams_[a] != 0 && teams_[a] == teams_[b];
}

void AdventureMoves::select(const model::MapActor* army, std::int32_t left) {
    selected_.reset();
    zones_.reset();
    field_.reset();
    danger_.clear();
    destination_.reset();
    day_              = 0;
    const auto actors = map_->actors();
    const auto index  = map_->index_of(army);
    if (!index || database_ == nullptr)
        return;
    selected_                           = *index;
    const model::CreatureCatalog& kinds = database_->creatures();
    const rules::ArmyMovement movement  = rules::army_movement(army->army, kinds);
    day_                                = movement.day;
    left_                               = std::min(left, day_);
    const rules::MapPoint source        = board_.origin(*selected_);

    const std::vector<std::uint32_t> enemies = visible_enemies(army->owner);
    const int stealth                        = rules::army_stealth(army->army);
    std::vector<rules::ZoneEnemy> marks;
    for (const std::uint32_t enemy : enemies) {
        const model::MapActor& other = actors[enemy];
        marks.push_back({board_.origin(enemy), !rules::sees_through(rules::army_detection(other.army, kinds), stealth)});
    }
    rules::PathField::Rules rules = land_search_rules(board_, *selected_, seen_);
    zones_                        = rules::EnemyZones::mark(rules::StepTest{board_, rules}, marks);
    rules.zones                   = &*zones_;
    field_                        = rules::PathField::search(board_, source, movement.terrain, rules);
    mark_danger(enemies);
}

std::vector<std::uint32_t> AdventureMoves::visible_enemies(std::uint8_t owner) const {
    std::vector<std::uint32_t> out;
    const auto actors = map_->actors();
    for (std::uint32_t i = 0; i < actors.size(); ++i) {
        const model::MapActor& other = actors[i];
        if (!board_.is_army(i) || friendly(owner, other.owner) || other.army.empty())
            continue;
        const rules::MapPoint at = board_.origin(i);
        if (seen_ != nullptr && seen_->level(at.x, at.y, at.level) != model::kFogVisible)
            continue;
        out.push_back(i);
    }
    return out;
}

void AdventureMoves::mark_danger(const std::vector<std::uint32_t>& enemies) {
    danger_.assign(2 * board_.tile_count(), false);
    const auto actors = map_->actors();
    for (const std::uint32_t enemy : enemies) {
        const model::MapActor& self        = actors[enemy];
        const rules::ArmyMovement movement = rules::army_movement(self.army, database_->creatures());
        rules::PathField::Rules rules;
        rules.blocks = [this, enemy, &actors](std::uint32_t actor) {
            return actor != enemy && board_.is_army(actor) && friendly(actors[enemy].owner, actors[actor].owner);
        };
        const rules::MapPoint from = board_.origin(enemy);
        rules.bound                = movement.day;
        rules.record_walled        = true;
        if (!self.owned() || !humans_[self.owner])
            rules.ground = rules::GuardGround::of(from, self.patrol.kind, self.patrol.radius);
        const rules::PathField reach = rules::PathField::search(board_, from, movement.terrain, rules);
        for (const rules::MapPoint& at : reach.reached())
            danger_[static_cast<std::size_t>(board_.slot(at))] = true;
    }
}

CursorWorld AdventureMoves::world(std::int32_t level) const {
    return CursorWorld{*map_,
                       board_,
                       [this](std::uint8_t a, std::uint8_t b) { return friendly(a, b); },
                       [seen = seen_, level](core::Point tile) {
                           return seen == nullptr || seen->level(tile.x, tile.y, level) != model::kFogHidden;
                       }};
}

std::optional<MapMover> AdventureMoves::mover() const {
    if (!selected_.has_value() || !field_.has_value())
        return std::nullopt;
    return MapMover{*selected_, &*field_, day_, left_, zones_.has_value() ? &*zones_ : nullptr};
}

std::optional<std::uint32_t> AdventureMoves::object_at(const scene::MapHover& hover) const {
    return object_under(world(hover.level), hover);
}

cursor::CursorState AdventureMoves::cursor_at(const scene::MapHover& hover) const {
    return map_cursor(world(hover.level), mover(), hover);
}

AdventureMoves::Click AdventureMoves::click(const scene::MapHover& hover) {
    const std::optional<MapMover> army = mover();
    std::optional<MoveTarget> target;
    if (army.has_value())
        target = move_target(world(hover.level), *army, hover);
    if (!target.has_value()) {
        destination_.reset();
        return {};
    }
    if (destination_ == target->to)
        return {.move = true};
    destination_ = target->to;

    return Click{marks_to(target->to)};
}

std::vector<scene::PathMark> AdventureMoves::marks_to(const rules::MapPoint& to) const {
    std::vector<scene::PathMark> out;
    const std::vector<rules::PathField::PathStep> steps = field_->path_to(to);
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const rules::PathField::PathStep& step = steps[i];
        scene::PathColour colour               = scene::PathColour::green;
        if (step.total > left_)
            colour = scene::PathColour::red;
        else if (danger_[static_cast<std::size_t>(board_.slot(step.at))])
            colour = scene::PathColour::yellow;
        out.push_back(scene::PathMark{{step.at.x, step.at.y},
                                      step.at.level,
                                      step.dir,
                                      i + 1 < steps.size() ? std::optional{steps[i + 1].dir} : std::nullopt,
                                      colour,
                                      step.at.bridge});
    }
    return out;
}

std::vector<scene::PathMark> AdventureMoves::keep_path(const rules::MapPoint& to) {
    destination_.reset();
    if (!field_.has_value() || !field_->total(to).has_value())
        return {};
    destination_ = to;
    return marks_to(to);
}

std::optional<std::int32_t> AdventureMoves::path_cost() const {
    if (!field_.has_value() || !destination_.has_value())
        return std::nullopt;
    return field_->total(*destination_);
}

AdventureMoves::Walk AdventureMoves::walk() const {
    Walk out;
    if (!selected_.has_value() || !field_.has_value() || !destination_.has_value())
        return out;
    rules::MapPoint from = board_.origin(*selected_);
    for (const rules::PathField::PathStep& step : field_->path_to(*destination_)) {
        if (step.total > left_) {
            out.out_of_movement = true;
            break;
        }
        if (const auto owner = rules::step_owner(board_, from, step.dir, rules::owns_every_step); owner.has_value()) {
            out.visit      = true;
            out.visit_step = step;
            out.visited    = *owner;
            break;
        }
        out.steps.push_back(step);
        from = step.at;
    }
    return out;
}

void AdventureMoves::army_moved(std::uint32_t actor, const rules::MapPoint& to) {
    board_.move_army(actor, to);
}

}
