#include "h4/app/new_game/scenario_options_controller.hpp"

#include <array>
#include <format>
#include <utility>

#include "h4/assets/assets.hpp"

namespace h4::app {

namespace {

constexpr std::array<std::string_view, 6> kColours{"Red", "Blue", "Green", "Orange", "Purple", "Teal"};
constexpr std::array<std::string_view, 6> kAlignments{"Life", "Order", "Death", "Chaos", "Nature", "Might"};
constexpr std::array<std::string_view, 5> kDifficultyNames{"new_map_difficulty_balloon_help_easy.misc",
                                                           "new_map_difficulty_balloon_help_normal.misc",
                                                           "new_map_difficulty_balloon_help_hard.misc",
                                                           "new_map_difficulty_balloon_help_expert.misc",
                                                           "new_map_difficulty_balloon_help_impossible.misc"};

}

ScenarioOptionsController::ScenarioOptionsController(screens::ScenarioOptionsView& view,
                                                     const assets::Assets& assets,
                                                     const model::ScenarioSummary& scenario,
                                                     bool guards_move,
                                                     Reporter report)
: view_{&view}, assets_{&assets}, report_{std::move(report)},
  setup_{model::ScenarioSetup::fill(scenario, std::string{assets.text("player.new_game")}, std::string{assets.text("computer.new_game")})
             .with_guards_move(guards_move)},
  description_{scenario.description} {
    conditions_ = std::format("{}{}\n\n{}{}",
                              assets.text("new_map_loss_condition.misc"),
                              scenario.loss_text ? std::string_view{*scenario.loss_text} : assets.text("default_loss_condition"),
                              assets.text("new_map_win_condition.misc"),
                              scenario.victory_text ? std::string_view{*scenario.victory_text} :
                                                      assets.text("default_victory_condition"));

    connect_view();
    view_->bind(model());
}

void ScenarioOptionsController::connect_view() {
    connections_.on(view_->button_clicked, [this](screens::OptionsButton button) {
        if (button == screens::OptionsButton::back)
            closed.emit();
        else
            begin();
    });
    connections_.on(view_->alignment_stepped,
                    [this](std::size_t row, int step) { apply(setup_.with_alignment_step(row, step)); });
    connections_.on(view_->colour_stepped,
                    [this](std::size_t row, int step) { apply(setup_.with_colour_step(row, step)); });
    connections_.on(view_->human_toggled, [this](std::size_t row, bool human) { apply(setup_.with_human(row, human)); });
    connections_.on(view_->difficulty_stepped, [this](int step) { apply(setup_.with_difficulty_step(step)); });
    connections_.on(view_->guards_toggled, [this](bool move) { apply(setup_.with_guards_move(move)); });
}

void ScenarioOptionsController::apply(model::ScenarioSetup setup) {
    setup_ = std::move(setup);
    view_->bind(model());
}

void ScenarioOptionsController::begin() {
    began.emit(setup_);
}

void ScenarioOptionsController::detach() noexcept {
    view_ = nullptr;
}

screens::ScenarioOptionsModel ScenarioOptionsController::model() const {
    screens::ScenarioOptionsModel out;
    for (std::size_t i = 0; i < setup_.rows().size(); ++i) {
        const model::SetupRow& row = setup_.rows()[i];
        screens::PlayerRowModel shown;
        shown.colour = kColours[std::to_underlying(row.colour)];
        shown.alignment_emblem = row.alignment ? kAlignments[std::to_underlying(*row.alignment)] : std::string_view{"Random"};
        shown.team_emblem      = std::format("team_{}", setup_.team(i));
        shown.name             = row.name;
        shown.human            = row.human;
        shown.human_capable    = setup_.human_capable(i);
        shown.colour_arrows    = setup_.colour_arrows_enabled(i);
        shown.alignment_arrows = setup_.alignment_arrows_enabled(i);
        out.rows.push_back(std::move(shown));
    }
    const auto difficulty = std::to_underlying(setup_.difficulty());
    out.difficulty_icon   = model::to_string(setup_.difficulty());
    out.difficulty_name   = assets_->text(kDifficultyNames[difficulty]);
    out.easier            = setup_.difficulty() != model::MapDifficulty::easy;
    out.harder            = setup_.difficulty() != model::MapDifficulty::impossible;
    out.guards_move       = setup_.guards_move();
    out.guards_caption    = assets_->text(setup_.guards_move() ? "guards_move.shared" : "guards_dont_move.shared");
    out.conditions        = conditions_;
    out.description       = description_;
    return out;
}

}
