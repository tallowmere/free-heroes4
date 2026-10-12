#include "h4/screens/new_game/scenario_details_view.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/button_table.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/dialog_frame.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kHelpWindow = "map_details";

constexpr std::array<ButtonSpec, 2> kButtons{{
    {.guide = "cancel", .art = ButtonArt::large},
    {.guide = "ok", .art = ButtonArt::large},
}};

CompositionRules details_rules() {
    CompositionRules rules{
        .help_window = kHelpWindow,
        .text_keys = {{"title_text", "details_title"}, {"humans_text", "human_players"}, {"map_type_text", "map_type.misc"}},
        .help_items     = {{"title_text", "", ""}},
        .text_alignment = {graphics::HorizontalAlign::left, graphics::VerticalAlign::centre},
    };
    add_buttons(rules, kButtons);
    return rules;
}

}

ScenarioDetailsView::ScenarioDetailsView(core::Size frame, ScenarioDetailsModel model)
: screens::ModalPopup{frame}, model_{std::move(model)} {
}

core::Result<std::unique_ptr<ScenarioDetailsView>>
ScenarioDetailsView::build(const assets::Assets& assets, core::Size frame, ScenarioDetailsModel model) {
    return compose_view<ScenarioDetailsView>(assets, frame, std::move(model));
}

core::Result<void> ScenarioDetailsView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.new_game_details", kHelpWindow, "details"});
    if (!opened)
        return std::unexpected{opened.error()};

    const ui::DialogFrameArt frame = opened->frame_art("dialog.generic");
    const core::Size size          = opened->layer("window_size").bounds().size();

    Builder dialog = opened->centred("new_game_details", HelpDecision::silent("0x7AE290 helps rows and buttons"), size);
    hold_dialog(dialog.parent());

    dialog.add<ui::DialogFrame>(HelpDecision::from_parent(), core::Rect{{}, size}, frame);
    dialog.compose(details_rules());

    rows_ = &dialog.add<ScenarioDetailsRows>(HelpDecision::from_parent(), dialog, model_.shown);

    buttons_ = {&dialog.button(kButtons[0]), &dialog.button(kButtons[1])};
    connections_.relay(buttons_[0]->clicked, cancelled);
    connections_.on(buttons_[1]->clicked, [this] { accepted.emit(choice()); });

    return dialog.finish();
}

ScenarioColumnSet ScenarioDetailsView::choice() const noexcept {
    return rows_->choice();
}

core::Rect ScenarioDetailsView::checkbox_rect(ScenarioColumn column) const noexcept {
    return rows_->checkbox_rect(column);
}

core::Rect ScenarioDetailsView::button_rect(DetailsButton button) const noexcept {
    return buttons_[std::to_underlying(button)]->rect_in_root();
}

}
