#include <format>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/visit_words.hpp"
#include "h4/assets/assets.hpp"

namespace h4::app {

void AdventureController::float_pickup(std::uint32_t object, const model::ResourceBundle& contents) {
    const model::MapActor& pile = scenario_.map().actors()[object];
    for (std::size_t r = 0; r < model::kResourceCount; ++r) {
        const auto resource       = static_cast<model::Resource>(r);
        const std::int32_t amount = contents[resource].value();
        if (amount == 0)
            continue;
        view_->map_window().float_text(pile.position,
                                       pile.level,
                                       std::format("+{} {}", amount, assets_->text(model::resource_word(resource))));
    }
}

core::Point AdventureController::above_object(const rules::PathField::PathStep& step) const {
    constexpr std::int32_t kAboveObject  = 0x40;
    const std::optional<core::Point> pin = view_->map_window().pin_in_root({step.at.x, step.at.y}, step.at.level);
    return pin ? *pin - core::Point{0, kAboveObject} : core::Point{};
}

void AdventureController::visit(std::uint32_t object, const rules::PathField::PathStep& step) {
    if (!game_ || !me_ || !face_walker(step))
        return;
    if (game_->shrine(object) != nullptr) {
        visit_shrine(object, step);
        settle_walk();
        show_heroes();
        return;
    }
    if (game_->chest_asks(journey_.actor(), object)) {
        chest_visit_ = ChestVisit{journey_.actor(), object, journey_.uncharged(step.total)};
        if (!messages_.is_open())
            open_chest(*chest_visit_);
        settle_walk();
        return;
    }
    pick_up(object, step);
}

void AdventureController::visit_shrine(std::uint32_t object, const rules::PathField::PathStep& step) {
    if (!game_ || !me_ || database_ == nullptr || !view_->has_map())
        return;
    const rules::ShrineState* shrine = game_->shrine(object);
    const model::Spell* spell = shrine != nullptr && shrine->spell ? database_->spells().spell(*shrine->spell) : nullptr;
    const rules::ShrineLesson lesson = game_->lesson(journey_.actor(), object);

    if (!apply(rules::commands::Visit{*me_, journey_.actor(), object, journey_.uncharged(step.total)}))
        return;
    if (spell == nullptr)
        return;

    const model::MapActor& army = scenario_.map().actors()[journey_.actor()];
    notice_over(step, shrine_words(*assets_, database_->texts(), scenario_.map().actors()[object], *spell, lesson, army));
}

bool AdventureController::face_walker(const rules::PathField::PathStep& step) {
    const rules::ArmyPlace* army = game_->army(journey_.actor());
    if (!view_->has_map() || army == nullptr)
        return false;
    scene::ArmyPose pose;
    if (const auto was = view_->map_window().army_pose(journey_.actor()); was.has_value())
        pose = *was;
    pose.facing = step.dir & 7;
    place_army(journey_.actor(), army->at, pose);
    return true;
}

void AdventureController::settle_walk() {
    path_.clear();
    present_map();
    select(selected_);
}

void AdventureController::pick_up(std::uint32_t object, const rules::PathField::PathStep& step) {
    std::optional<rules::Artifact> found;
    if (const rules::Treasure* treasure = game_->treasure(object); treasure != nullptr)
        found = treasure->artifact;
    auto next = game_->apply(rules::commands::Visit{*me_, journey_.actor(), object, journey_.uncharged(step.total)});
    if (!next.has_value()) {
        const model::MapActor& target = scenario_.map().actors()[object];
        report_.not_ported(std::format("visiting {}", target.family.empty() ? target.model : target.family));
        select(selected_);
        return;
    }
    game_ = std::move(*next);
    take_object(object);
    if (found.has_value())
        found_artifact(object, step, *found);
    settle_walk();
    show_purse();
    show_heroes();
}

void AdventureController::take_object(std::uint32_t object) {
    const rules::Treasure* treasure = game_->treasure(object);
    if (treasure == nullptr || !treasure->taken)
        return;
    if (moves_)
        moves_->object_taken(object);
    view_->map_window().remove_object(object);
    if (!treasure->chest)
        float_pickup(object, treasure->contents);
}

}
