#include "h4/screens/new_game/scenario_options_view.hpp"

#include <array>
#include <memory>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/text.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/button_table.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/text_window.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kProseFontSize  = 18;
constexpr std::string_view kHelpWindow = "file_open";

constexpr std::array<ButtonSpec, 2> kButtons{{
    {"Back_Button", "back.shared", "scenario_information", "close"},
    {"Begin_Button", "begin"},
}};

constexpr std::array<ButtonSpec, 2> kDifficultyArrows{{
    {"Left", {}, "campaign", "player_difficulty_left"},
    {"Right", {}, "campaign", "player_difficulty_right"},
}};

constexpr std::array<CaptionSpec, 4> kColumnTitles{{
    {"Name_Title", "name.new_game"},
    {"Team_Title", "new_map_team.misc"},
    {"Alignment_Title", "alignment.new_game"},
    {"Color_Title", "color.new_game"},
}};

constexpr ui::LabelStyle kEngravedTop{graphics::TextStyle::engraved(),
                                      {graphics::HorizontalAlign::centre, graphics::VerticalAlign::top}};

CompositionRules options_rules() {
    CompositionRules rules{
        .help_window      = kHelpWindow,
        .placed_by_screen = {"Creature_Guard", "Player_Background", "Difficulty_Text"},
        .text_alignment   = {graphics::HorizontalAlign::centre, graphics::VerticalAlign::top},
    };
    add_buttons(rules, kButtons);
    add_buttons(rules, kDifficultyArrows);
    return rules;
}

}

ScenarioOptionsView::ScenarioOptionsView(core::Size frame, ScenarioOptionsModel model)
: screens::ModalPopup{frame}, model_{std::move(model)} {
}

core::Result<std::unique_ptr<ScenarioOptionsView>>
ScenarioOptionsView::build(const assets::Assets& assets, core::Size frame, ScenarioOptionsModel model) {
    return compose_view<ScenarioOptionsView>(assets, frame, std::move(model));
}

core::Result<void> ScenarioOptionsView::compose(const assets::Assets& assets) {
    assets_     = &assets;
    auto opened = Builder::open(tree().root(), assets, {"dialog.new_game_options", kHelpWindow, "scenario options"});
    if (!opened)
        return std::unexpected{opened.error()};
    difficulty_icons_ = &opened->group("icons.difficulty_60");

    Builder dialog = opened->centred("new_game_options", HelpDecision::silent("0x7C1E20 helps its widgets alone"));
    hold_dialog(dialog.parent());

    dialog.compose(options_rules());
    dialog.captions(kColumnTitles, kEngravedTop);
    add_buttons(dialog);
    add_difficulty(dialog);
    add_prose(dialog);
    add_guards(dialog);
    add_rows(dialog);
    bind(model_);
    return dialog.finish();
}

void ScenarioOptionsView::add_buttons(Builder& dialog) {
    for (std::size_t i = 0; i < kButtons.size(); ++i) {
        buttons_[i] = &dialog.button(kButtons[i]);
        connections_.relay(buttons_[i]->clicked, button_clicked, static_cast<OptionsButton>(i));
    }
}

void ScenarioOptionsView::add_difficulty(Builder& dialog) {
    easier_ = &dialog.button(kDifficultyArrows[0]);
    harder_ = &dialog.button(kDifficultyArrows[1]);
    connections_.relay(easier_->clicked, difficulty_stepped, -1);
    connections_.relay(harder_->clicked, difficulty_stepped, 1);
    difficulty_icon_ = &dialog.add<ui::IconCell>(
        HelpDecision::from_parent(), dialog.guide("Player_Difficulty"), nullptr, nullptr, ui::IconCell::Placement::authored
    );
    const core::Rect words = dialog.guide("Difficulty_Text");
    difficulty_text_ = &dialog.label(words, {}, ui::kEngravedCentred, HelpDecision::from_parent(), words.height() / 2);
}

void ScenarioOptionsView::add_prose(Builder& dialog) {
    const ui::ScrollbarArt bar = dialog.scrollbar_art("vertical_scroll");
    const formats::Font& prose = dialog.font(kProseFontSize);
    const auto quiet           = HelpDecision::silent("0x7C1E20 helps no prose window");
    description_ = &dialog.add<ui::TextWindow>(quiet, dialog.guide("Map_Description"), std::string{}, prose, ui::LabelStyle{}, bar);
    conditions_ = &dialog.add<ui::TextWindow>(quiet, dialog.guide("victory_loss_Condition"), std::string{}, prose, ui::LabelStyle{}, bar);
}

void ScenarioOptionsView::add_guards(Builder& dialog) {
    const core::Rect words = dialog.guide("creature_guard_text");
    guards_text_           = &dialog.label(words, {}, kEngravedTop, HelpDecision::from_parent(), words.height() / 2);
    guards_ = &dialog.add<ui::ToggleButton>(dialog.help("shared", "guards_released"), core::Point{}, dialog.faces("Creature_Guard"));
    connections_.relay(guards_->toggled, guards_toggled);
}

void ScenarioOptionsView::add_rows(Builder& dialog) {
    rows_ = &dialog.add<PlayerRowList>(HelpDecision::from_parent(), dialog);
    connections_.relay(rows_->alignment_stepped, alignment_stepped);
    connections_.relay(rows_->colour_stepped, colour_stepped);
    connections_.relay(rows_->human_toggled, human_toggled);
}

void ScenarioOptionsView::bind(const ScenarioOptionsModel& model) {
    model_ = model;
    conditions_->set_text(model_.conditions);
    description_->set_text(model_.description);
    difficulty_icon_->set_layer(difficulty_icons_->find(model_.difficulty_icon), ui::IconCell::Placement::authored);
    difficulty_text_->set_text(model_.difficulty_name);
    easier_->set_enabled(model_.easier);
    harder_->set_enabled(model_.harder);
    guards_->set_checked(model_.guards_move);
    guards_text_->set_text(model_.guards_caption);
    guards_->set_help(interface_help(*assets_, "shared", model_.guards_move ? "guards_pressed" : "guards_released"));
    rows_->bind(model_.rows);
}

core::Rect ScenarioOptionsView::button_rect(OptionsButton button) const noexcept {
    return ui::shown_rect(buttons_[std::to_underlying(button)]);
}

core::Rect ScenarioOptionsView::checkbox_rect(std::size_t row) const noexcept {
    return rows_->checkbox_rect(row);
}

core::Rect ScenarioOptionsView::row_rect(std::size_t row) const noexcept {
    return rows_->row_rect(row);
}

core::Rect ScenarioOptionsView::alignment_arrow_rect(std::size_t row, OptionsArrow arrow) const noexcept {
    return rows_->alignment_arrow_rect(row, arrow == OptionsArrow::up);
}

core::Rect ScenarioOptionsView::colour_arrow_rect(std::size_t row, OptionsArrow arrow) const noexcept {
    return rows_->colour_arrow_rect(row, arrow == OptionsArrow::up);
}

core::Rect ScenarioOptionsView::difficulty_arrow_rect(OptionsArrow arrow) const noexcept {
    return ui::shown_rect(arrow == OptionsArrow::up ? easier_ : harder_);
}

core::Rect ScenarioOptionsView::guards_rect() const noexcept {
    return ui::shown_rect(guards_);
}

std::string_view ScenarioOptionsView::difficulty_text() const noexcept {
    return difficulty_text_->text();
}

std::string_view ScenarioOptionsView::guards_text() const noexcept {
    return guards_text_->text();
}

core::Rect ScenarioOptionsView::conditions_scrollbar_rect() const noexcept {
    return ui::shown_rect(conditions_->scrollbar());
}

core::Rect ScenarioOptionsView::description_scrollbar_rect() const noexcept {
    return ui::shown_rect(description_->scrollbar());
}

}
