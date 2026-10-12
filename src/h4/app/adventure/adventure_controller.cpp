#include "h4/app/adventure/adventure_controller.hpp"

#include <format>
#include <limits>
#include <utility>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/rules/calendar.hpp"

namespace h4::app {

AdventureController::AdventureController(std::unique_ptr<screens::AdventureView> view,
                                         const assets::Assets& assets,
                                         GameOpening opening,
                                         MenuFactory menu,
                                         Reporter report,
                                         const model::GameDatabase* database)
: scenario_{std::move(opening.scenario)}, view_{std::move(view)}, host_{*view_}, assets_{&assets},
  setup_{std::move(opening.setup)}, database_{database}, menu_{std::move(menu)}, report_{std::move(report)},
  system_menu_{system_menu_entries()} {
    connect_view();
    game_      = std::move(opening.game);
    me_        = opening.me;
    shown_.day = date_words(assets.text("date.adventure_frame"), game_ ? game_->calendar() : rules::Calendar{});
    show_purse();

    seat_viewer();
    show_map();
    show_heroes();

    for (const model::SetupRow& row : setup_.rows()) {
        if (!row.human)
            continue;
        for (std::string& words : scenario_.opening_messages(row.colour, true))
            messages_.add(std::move(words));
        break;
    }
    messages_.show_first();
}

void AdventureController::set_settings(model::GameSettings settings) noexcept {
    settings_ = settings;
    set_frame_policy(frame_policy_for(settings_));
}

void AdventureController::on_resized(core::Size frame) {
    end_walk();
    auto rebuilt = screens::AdventureView::build(*assets_, frame);
    if (report_.failed(rebuilt))
        return;
    const bool showing = messages_.is_open();
    const bool had_map        = view_->has_map();
    const core::Point centre  = view_->map_window().centre();
    const std::uint32_t clock = view_->clock_ms();
    host_.rebase(view_, std::move(*rebuilt));
    connect_view();
    shown_.day = date_words(assets_->text("date.adventure_frame"), game_ ? game_->calendar() : rules::Calendar{});
    view_->set_clock(clock);
    if (had_map) {
        show_map();
        view_->map_window().centre_on(centre);
    }
    show_purse();
    show_heroes();
    system_scroll_.forget();
    settings_popup_.forget();
    messages_.forget();
    question_.forget();
    chest_.forget();
    army_window_.forget();
    if (showing)
        messages_.show_first();
    if (chest_visit_)
        open_chest(*chest_visit_);
}

void AdventureController::seat_viewer() {
    std::uint8_t team = 0;
    for (std::size_t row = 0; row < setup_.rows().size(); ++row) {
        if (!setup_.rows()[row].human)
            continue;
        viewer_                  = setup_.rows()[row].colour;
        const std::size_t shared = setup_.team(row);
        for (std::size_t other = 0; other < setup_.rows().size(); ++other)
            if (setup_.team(other) == shared)
                team |= static_cast<std::uint8_t>(1U << std::to_underlying(setup_.rows()[other].colour));
        break;
    }
    shroud_.emplace(*assets_, scenario_.map(), viewer_, team);
}

void AdventureController::show_map() {
    view_->show_map(*assets_, scenario_.map(), seen());
    moves_.emplace(*assets_, scenario_.map(), setup_, database_, seen());
    sync_armies();
    select_first();
}

const model::Shroud* AdventureController::seen() const noexcept {
    return shroud_ ? shroud_->plane() : nullptr;
}

void AdventureController::select_first() {
    for (const model::SetupRow& row : setup_.rows()) {
        if (!row.human)
            continue;
        const std::vector<const model::MapActor*> mine = scenario_.map().owned_by(row.colour);
        for (model::ActorKind kind : {model::ActorKind::hero, model::ActorKind::town})
            for (const model::MapActor* actor : mine)
                if (actor->kind == kind) {
                    centre_on_actor(*actor);
                    if (kind == model::ActorKind::hero)
                        select(actor);
                    return;
                }
        break;
    }
}

model::ResourceBundle AdventureController::purse() const noexcept {
    return game_ && me_ ? game_->purse(*me_) : model::ResourceBundle{};
}

void AdventureController::show_purse() {
    shown_.resources = purse_amounts(purse());
    present();
}

void AdventureController::show_heroes() {
    const std::vector<const model::MapActor*> armies = player_armies(scenario_.map(), viewer_);
    shown_.heroes                                    = hero_list_entries(*assets_, armies, database_, movement_left());
    shown_.hero                                      = row_of(armies, selected_);
    shown_.rings                                     = army_ring_entries(*assets_, selected_, database_);
    if (!ring_)
        ring_ = default_ring(selected_);
    shown_.ring = ring_;
    shown_.strip = ring_ ? strip_entry(*assets_, selected_, *ring_, database_, spent_of(selected_)) : screens::StatEntry{};
    const std::vector<const model::MapActor*> towns = player_towns(scenario_.map(), viewer_);
    shown_.towns                                    = town_list_entries(*assets_, scenario_.map(), towns);
    shown_.town.reset();
    present();
}

void AdventureController::present() {
    view_->bind({
        .heroes        = shown_.heroes,
        .hero_selected = shown_.hero,
        .towns         = shown_.towns,
        .town_selected = shown_.town,
        .rings         = shown_.rings,
        .ring_selected = shown_.ring,
        .strip         = shown_.strip,
        .resources     = shown_.resources,
        .day           = shown_.day,
    });
}

void AdventureController::present_map() {
    view_->map_window().bind({.selected = selected_, .path = path_});
}

void AdventureController::on_button(screens::AdventureButton button) {
    if (button == screens::AdventureButton::system_menu) {
        open_system_menu();
        return;
    }
    if (button == screens::AdventureButton::end_turn) {
        ask_end_turn();
        return;
    }
    report_.not_ported(std::string{screens::button_spec(button).help_item});
}

bool AdventureController::commit(core::Result<rules::GameState> next) {
    if (report_.failed(next))
        return false;
    game_ = std::move(*next);
    return true;
}

}
