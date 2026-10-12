#include <string>
#include <utility>

#include "h4/ai/passive_controller.hpp"
#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"

namespace h4::app {

void AdventureController::ask_end_turn() {
    if (!game_ || !me_ || game_->current_player() != *me_ || journey_.walking() || host_.has_popup())
        return;
    if (!settings_.army_movement_reminder || !armies_can_move()) {
        end_turn();
        return;
    }
    const core::Rect button = view_->button_rect(screens::AdventureButton::end_turn);
    auto dialog             = screens::BasicDialog::build(
        *assets_,
        view_->frame_size(),
        screens::BasicDialogContent{.text    = std::string{assets_->text("movement_left.misc")},
                                    .buttons = screens::DialogButtons::check_and_cross},
        screens::DialogPlacement::standing_on(button.centre())
    );
    if (report_.failed(dialog))
        return;
    question_.open(std::move(*dialog),
                   [this](screens::BasicDialog& question, core::Connections& wiring) {
                       wiring.on(question.answered, [this](bool yes) {
                           close_question();
                           if (yes)
                               end_turn();
                       });
                   },
                   {[this] { close_question(); },
                    [this] {
                        close_question();
                        end_turn();
                    }});
}

void AdventureController::close_question() {
    question_.close();
}

bool AdventureController::armies_can_move() const {
    const auto actors = scenario_.map().actors();
    for (const rules::ArmyPlace& army : game_->armies()) {
        if (army.owner != me_ || army.left <= 0)
            continue;
        std::int32_t path = 0;
        if (moves_ && selected_ == &actors[army.actor])
            path = moves_->path_cost().value_or(0);
        if (path <= army.left)
            return true;
    }
    return false;
}

void AdventureController::end_turn() {
    path_.clear();
    present_map();
    ai::PassiveController computer;
    auto next = game_->apply(rules::commands::EndTurn{*me_});
    while (next.has_value() && next->current_player() != *me_) {
        const rules::PlayerId player = next->current_player();
        if (setup_.rows()[player.value()].human) {
            report_.not_ported("Another human player's turn");
            next = next->apply(rules::commands::EndTurn{player});
            continue;
        }
        next = next->apply(computer.decide(*next, player));
    }
    if (!commit(std::move(next)))
        return;
    begin_turn();
}

void AdventureController::begin_turn() {
    shown_.day = date_words(assets_->text("date.adventure_frame"), game_->calendar());
    show_purse();
    select_first();
    show_heroes();
    const model::SetupRow& row = setup_.rows()[me_->value()];
    messages_.clear();
    for (std::string& words : scenario_.messages_on(game_->calendar().day().value(), row.colour, true))
        messages_.add(std::move(words));
    messages_.show_first();
}

}
