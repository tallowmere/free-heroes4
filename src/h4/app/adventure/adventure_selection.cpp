#include <vector>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/app/adventure/map_words.hpp"

namespace h4::app {

void AdventureController::on_hero_row(std::size_t row) {
    if (journey_.walking())
        return;
    const std::vector<const model::MapActor*> armies = player_armies(scenario_.map(), viewer_);
    if (row >= armies.size())
        return;
    select(armies[row]);
    ring_ = default_ring(selected_);
    show_heroes();
    centre_on_actor(*selected_);
}

void AdventureController::on_hero_row_double_clicked(std::size_t row) {
    const std::vector<const model::MapActor*> armies = player_armies(scenario_.map(), viewer_);
    if (row >= armies.size() || armies[row] != selected_)
        return;
    open_army_window(ring_);
}

void AdventureController::on_town_row(std::size_t row) {
    const std::vector<const model::MapActor*> towns = player_towns(scenario_.map(), viewer_);
    if (row >= towns.size())
        return;
    shown_.hero.reset();
    shown_.town = row;
    present();
    centre_on_actor(*towns[row]);
}

void AdventureController::select(const model::MapActor* army) {
    selected_ = army;
    if (moves_) {
        if (army != nullptr)
            moves_->select(army, left_of(*army));
        else
            moves_->select(nullptr);
        path_.clear();
    }
    present_map();
    if (const std::optional<scene::MapHover> hover = view_->map_window().hover(); hover.has_value())
        on_map_pointed(*hover);
}

ui::Help AdventureController::map_words_at(const scene::MapHover& hover) const {
    const std::optional<std::uint32_t> object = moves_ ? moves_->object_at(hover) : std::nullopt;
    return map_words(*assets_, database_, scenario_.map(), seen(), hover, object, settings_.show_coordinates);
}

void AdventureController::on_map_pointed(const scene::MapHover& hover) {
    if (moves_)
        view_->map_window().set_map_cursor(moves_->cursor_at(hover));
}

void AdventureController::connect_view() {
    connections_.clear();
    connections_.on(view_->button_clicked, [this](screens::AdventureButton button) { on_button(button); });
    connections_.on(view_->mini_map_pointed, [this](core::Point pixel) { view_->map_window().centre_on(pixel); });
    connections_.on(view_->ring_pressed, [this](std::size_t ring) { on_ring(ring); });
    connections_.on(view_->ring_double_clicked, [this](std::size_t ring) { open_army_window(ring); });
    connections_.on(view_->hero_row_pressed, [this](std::size_t row) { on_hero_row(row); });
    connections_.on(view_->hero_row_double_clicked, [this](std::size_t row) { on_hero_row_double_clicked(row); });
    connections_.on(view_->town_row_pressed, [this](std::size_t row) { on_town_row(row); });
    connections_.on(view_->map_pointed, [this](const scene::MapHover& hover) { on_map_pointed(hover); });
    connections_.on(view_->map_clicked, [this](const scene::MapHover& hover) { on_map_clicked(hover); });
    connections_.on(view_->ticked, [this](std::uint32_t now) { on_ticked(now); });
    view_->map_window().set_words([this](const scene::MapHover& hover) { return map_words_at(hover); });
}

void AdventureController::on_map_clicked(const scene::MapHover& hover) {
    if (!moves_)
        return;
    if (journey_.walking()) {
        journey_.stop();
        end_walk();
        return;
    }
    const AdventureMoves::Click click = moves_->click(hover);
    if (click.move) {
        start_walk();
        return;
    }
    path_ = click.marks;
    present_map();
}

void AdventureController::centre_on_actor(const model::MapActor& actor) {
    if (const rules::ArmyPlace* place = placement_of(&actor); place != nullptr)
        view_->map_window().look_at(place->at.x, place->at.y);
    else
        view_->map_window().look_at(actor);
}

void AdventureController::on_ring(std::size_t ring) {
    const screens::StatEntry entry = strip_entry(*assets_, selected_, ring, database_, spent_of(selected_));
    if (!entry.shown)
        return;
    ring_        = ring;
    shown_.ring  = ring_;
    shown_.strip = entry;
    present();
}

}
