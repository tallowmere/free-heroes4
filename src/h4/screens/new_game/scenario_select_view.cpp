#include "h4/screens/new_game/scenario_select_view.hpp"

#include <array>
#include <format>
#include <memory>
#include <utility>

#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/button_table.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/text_window.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kDescriptionFontSize = 16;
constexpr std::string_view kHelpWindow      = "file_open";

constexpr std::array<ButtonSpec, 3> kButtons{{
    {"Back_Button", "cancel", "shared", "cancel"},
    {"Details_Button", "details", kHelpWindow, "details"},
    {"Begin_Button", "new_map_next.misc", "shared", "next"},
}};

CompositionRules select_rules() {
    CompositionRules rules{
        .help_window      = kHelpWindow,
        .placed_by_screen = {"Maps", "Vertical_Line"},
        .text_alignment   = {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre},
    };
    for (const std::string_view faces : kColumnFaces)
        rules.placed_by_screen.push_back(faces);
    add_buttons(rules, kButtons);
    return rules;
}

}

ScenarioSelectView::ScenarioSelectView(core::Size frame, ScenarioSelectModel model)
: screens::ModalPopup{frame}, model_{std::move(model)} {
}

core::Result<std::unique_ptr<ScenarioSelectView>>
ScenarioSelectView::build(const assets::Assets& assets, core::Size frame, ScenarioSelectModel model) {
    return compose_view<ScenarioSelectView>(assets, frame, std::move(model));
}

core::Result<void> ScenarioSelectView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.new_game", kHelpWindow, "scenario select"});
    if (!opened)
        return std::unexpected{opened.error()};

    const auto layout = ScenarioSelectLayout::from_group(opened->group());
    if (!layout)
        return std::unexpected{layout.error()};
    const auto silent = HelpDecision::silent("0x7B2B30 helps headers, buttons and rows alone");

    Builder dialog = opened->centred("new_game", silent, layout->bounds().size());
    hold_dialog(dialog.parent());
    dialog.compose(select_rules());
    for (std::size_t i = 0; i < kButtons.size(); ++i)
        buttons_[i] = &dialog.button(kButtons[i]);

    list_ = &dialog.add<ScenarioList>(HelpDecision::from_parent(), dialog, *layout);
    connections_.relay(list_->row_selected, row_selected);
    connections_.relay(list_->row_activated, row_activated);
    connections_.relay(list_->sort_requested, sort_requested);

    description_ = &dialog.add<ui::TextWindow>(
        silent, layout->description, std::string{}, dialog.font(kDescriptionFontSize), ui::LabelStyle{}, dialog.scrollbar_art("vertical_scroll")
    );
    description_title_ = dialog.text("scene_info_description_title.misc");
    for (std::size_t i = 0; i < buttons_.size(); ++i)
        connections_.relay(buttons_[i]->clicked, button_clicked, static_cast<ScenarioButton>(i));

    bind(model_);
    return dialog.finish();
}

void ScenarioSelectView::bind(const ScenarioSelectModel& model) {
    model_ = model;
    list_->bind(model_.rows, model_.columns, model_.selected);
    description_->set_text(std::format("{}:\n{}", description_title_, model_.description));
    buttons_[2]->set_enabled(model_.next_enabled);
}

core::Rect ScenarioSelectView::slot_rect(std::size_t index) const noexcept {
    return list_ != nullptr ? list_->slot_rect(index) : core::Rect{};
}

core::Rect ScenarioSelectView::header_rect(ScenarioColumn column) const noexcept {
    return list_ != nullptr ? list_->header_rect(column) : core::Rect{};
}

core::Rect ScenarioSelectView::scrollbar_rect() const noexcept {
    return list_ != nullptr ? list_->scrollbar_rect() : core::Rect{};
}

core::Rect ScenarioSelectView::button_rect(ScenarioButton button) const noexcept {
    const std::size_t index = std::to_underlying(button);
    return index < buttons_.size() && buttons_[index] != nullptr ? buttons_[index]->rect_in_root() : core::Rect{};
}

std::size_t ScenarioSelectView::row_count() const noexcept {
    return list_ != nullptr ? list_->row_count() : 0;
}

std::size_t ScenarioSelectView::selected() const noexcept {
    return list_ != nullptr ? list_->selected() : 0;
}

std::size_t ScenarioSelectView::first_visible() const noexcept {
    return list_ != nullptr ? list_->first_visible() : 0;
}

bool ScenarioSelectView::next_enabled() const noexcept {
    return model_.next_enabled;
}

ScenarioColumnSet ScenarioSelectView::columns() const noexcept {
    return model_.columns;
}

}
