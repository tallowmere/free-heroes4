#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/app/adventure/army_window_content.hpp"

namespace h4::app {

namespace {

std::string_view control_name(screens::ArmyWindowView::Control control) noexcept {
    using Control = screens::ArmyWindowView::Control;
    switch (control) {
    case Control::dismiss: return "Dismissing a hero";
    case Control::split: return "Splitting an army";
    case Control::spellbook: return "The spellbook";
    case Control::hide_potions: return "Hiding potions";
    case Control::loose:
    case Control::tight:
    case Control::square: return "Army formations";
    case Control::move_army_up:
    case Control::move_army_down: return "Moving an army between rows";
    }
    return "?";
}

}

void AdventureController::open_army_window(std::optional<std::size_t> ring) {
    if (selected_ == nullptr || journey_.walking() || !game_)
        return;
    auto content = army_window_model(ring);
    if (!content)
        return;
    auto window = screens::ArmyWindowView::build(*assets_, view_->frame_size(), std::move(*content));
    if (report_.failed(window))
        return;
    army_window_.open(std::move(*window),
                      [this](screens::ArmyWindowView& opened, core::Connections& wiring) {
                          wiring.on(opened.closed, [this] { close_army_window(); });
                          wiring.on(opened.ring_pressed, [this](std::size_t pressed) {
                              close_army_window();
                              open_army_window(pressed);
                          });
                          wiring.on(opened.army_pressed, [this](std::size_t row) { turn_army_window(row); });
                          wiring.on(opened.button_clicked, [this](screens::ArmyWindowView::Control control) {
                              report_.not_ported(control_name(control));
                          });
                      },
                      {[this] { close_army_window(); }, [this] { close_army_window(); }});
}

std::optional<screens::ArmyWindowModel> AdventureController::army_window_model(std::optional<std::size_t> ring) const {
    const auto index = scenario_.map().index_of(selected_);
    if (!index)
        return std::nullopt;
    const rules::HeroState* hero = nullptr;
    for (const rules::HeroState& h : game_->heroes_of(*index))
        if (ring && h.slot == *ring)
            hero = &h;
    screens::ArmyWindowModel content = army_window_content(*assets_, *selected_, ring, hero, database_, spent_of(selected_));
    const std::vector<const model::MapActor*> armies = player_armies(scenario_.map(), viewer_);
    content.armies                                   = hero_list_entries(*assets_, armies, database_, movement_left());
    content.army_row                                 = row_of(armies, selected_);
    return content;
}

void AdventureController::turn_army_window(std::size_t row) {
    const std::vector<const model::MapActor*> armies = player_armies(scenario_.map(), viewer_);
    if (row >= armies.size())
        return;
    close_army_window();
    select(armies[row]);
    ring_ = default_ring(selected_);
    show_heroes();
    open_army_window(ring_);
}

void AdventureController::close_army_window() {
    army_window_.close();
}

}
