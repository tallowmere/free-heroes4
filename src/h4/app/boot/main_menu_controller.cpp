#include "h4/app/boot/main_menu_controller.hpp"

#include <format>
#include <string>
#include <utility>

#include "h4/app/boot/game_settings_popup.hpp"
#include "h4/app/boot/scroll_menu.hpp"
#include "h4/assets/assets.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::app {

namespace {

std::string_view display_name(screens::MainMenuAction action) noexcept {
    switch (action) {
    case screens::MainMenuAction::new_game: return "New Game";
    case screens::MainMenuAction::load_game: return "Load Game";
    case screens::MainMenuAction::options: return "Options";
    case screens::MainMenuAction::quit: return "Quit";
    case screens::MainMenuAction::multiplayer: return "Multiplayer";
    }
    return "?";
}

}

MainMenuController::MainMenuController(std::unique_ptr<screens::MainMenuView> view,
                                       const assets::Assets& assets,
                                       model::InstalledContent content,
                                       std::filesystem::path maps_dir,
                                       Reporter report,
                                       const model::GameDatabase* database)
: view_{std::move(view)}, host_{*view_}, assets_{&assets}, database_{database}, content_{content},
  maps_dir_{std::move(maps_dir)}, report_{std::move(report)} {
    model_ = make_model(true);
    view_->bind(model_);
    connect_view();
}

void MainMenuController::connect_view() {
    action_connection_ = view_->action.connect([this](screens::MainMenuAction action) { on_action(action); });
}

void MainMenuController::on_action(screens::MainMenuAction action) {
    switch (action) {
    case screens::MainMenuAction::new_game: open_new_game_menu(); break;
    case screens::MainMenuAction::options: open_options_menu(); break;
    case screens::MainMenuAction::quit: request(transitions::Quit{}); break;
    default: report_.not_ported(display_name(action)); break;
    }
}

void MainMenuController::open_scroll(screens::MainMenuAction button,
                                     std::span<const std::string_view> keys,
                                     std::function<void(std::size_t)> chosen) {
    const auto scroll = open_scroll_menu(scroll_, *assets_, view_->frame_size(), action_anchor(button), keys, std::move(chosen));
    if (report_.failed(scroll))
        return;
    scroll_button_ = button;
}

bool MainMenuController::scroll_open(screens::MainMenuAction button) const noexcept {
    return scroll_.is_open() && scroll_button_ == button;
}

void MainMenuController::open_new_game_menu() {
    new_game_entries_ = new_game_entries(content_);
    open_scroll(screens::MainMenuAction::new_game,
                text_keys(std::span<const NewGameEntry>{new_game_entries_}),
                [this](std::size_t index) { on_new_game_choice(index); });
}

void MainMenuController::on_new_game_choice(std::size_t index) {
    scroll_.close();
    if (index >= new_game_entries_.size())
        return;
    if (new_game_entries_[index].choice == NewGameChoice::scenarios)
        open_scenario_select();
    else if (new_game_entries_[index].choice == NewGameChoice::tutorial)
        open_tutorial();
    else
        report_.not_ported(display_name(new_game_entries_[index].choice));
}

void MainMenuController::open_options_menu() {
    options_entries_ = options_entries(content_);
    open_scroll(screens::MainMenuAction::options,
                text_keys(std::span<const OptionsEntry>{options_entries_}),
                [this](std::size_t index) { on_options_choice(index); });
}

void MainMenuController::on_options_choice(std::size_t index) {
    scroll_.close();
    if (index >= options_entries_.size())
        return;
    if (options_entries_[index].choice == OptionsChoice::game_settings)
        open_game_settings();
    else
        report_.not_ported(display_name(options_entries_[index].choice));
}

void MainMenuController::set_settings(model::GameSettings settings) noexcept {
    settings_ = settings;
    set_frame_policy(frame_policy_for(settings_));
}

void MainMenuController::open_scenario_select() {
    auto view = screens::ScenarioSelectView::build(*assets_, view_->frame_size(), screens::ScenarioSelectModel{});
    if (report_.failed(view))
        return;
    auto& select    = **view;
    auto host       = std::make_unique<screens::PopupStack>(std::move(*view));
    auto controller = std::make_unique<ScenarioSelectController>(select, host->host(), *assets_, report_);
    controller->set_guards_move(settings_.guards_move);
    if (report_.failed(controller->open_folder(maps_dir_)))
        return;
    set_actions_enabled(false);
    select_.open(std::move(host),
                 std::move(controller),
                 [this](screens::PopupStack&, ScenarioSelectController& opened, core::Connections& wiring) {
                     wiring.on(opened.closed, [this] { close_scenario_select(); });
                     wiring.on(opened.began, [this](std::filesystem::path file, model::ScenarioSetup setup) {
                         start_scenario(std::move(file), std::move(setup));
                     });
                 },
                 {[this] { close_scenario_select(); }, {}});
}

void MainMenuController::close_scenario_select() {
    select_.close();
    set_actions_enabled(true);
}

void MainMenuController::on_resized(core::Size frame) {
    const auto resolution = screens::resolution_for_width(frame.width);
    if (!resolution)
        return;
    const auto size = screens::size_of(*resolution);
    auto model      = make_model(true);
    auto rebuilt    = screens::MainMenuView::build(*assets_, size, model);
    if (report_.failed(rebuilt))
        return;
    model_ = std::move(model);
    select_.forget();
    settings_popup_.forget();
    tutorial_.forget();
    loading_.forget();
    load_.reset();
    scroll_.forget();
    host_.rebase(view_, std::move(*rebuilt));
    view_->bind(model_);
    connect_view();
}

std::function<std::unique_ptr<Controller>(const model::GameSettings&)> MainMenuController::menu_factory() const {
    return [assets   = assets_,
            content  = content_,
            maps     = maps_dir_,
            report   = report_,
            saver    = saver_,
            database = database_,
            size     = view_->frame_size()](const model::GameSettings& settings) -> std::unique_ptr<Controller> {
        const auto resolution = screens::resolution_for_width(size.width);
        if (!resolution)
            return nullptr;
        const auto frame_size = screens::size_of(*resolution);
        auto view             = screens::MainMenuView::build(*assets, frame_size, screens::MainMenuModel{});
        if (!view)
            return nullptr;
        auto controller = std::make_unique<MainMenuController>(std::move(*view), *assets, content, maps, report, database);
        controller->set_settings(settings);
        controller->set_settings_saver(saver);
        return controller;
    };
}

void MainMenuController::on_key(platform::Key key) {
    if (settings_popup_.on_key(key) || tutorial_.on_key(key))
        return;
    if (select_.is_open() && select_.controller()->on_key(key))
        return;
    if (select_.on_key(key) || scroll_.on_key(key) || loading_.on_key(key))
        return;
    if (key == platform::Key::escape)
        request(transitions::Quit{});
}

void MainMenuController::on_frame() {
    if (loading_.is_open())
        step_loading();
    if (select_.is_open())
        select_.controller()->on_frame();
}

void MainMenuController::advance(core::Instant now) {
    now_ = now;
}

core::Point MainMenuController::action_anchor(screens::MainMenuAction action) const {
    return view_->action_art_rect(action).origin();
}

void MainMenuController::set_actions_enabled(bool enabled) {
    model_.actions_enabled = enabled;
    view_->bind(model_);
}

screens::MainMenuModel MainMenuController::make_model(bool enabled) const {
    return screens::MainMenuModel{.actions_enabled = enabled};
}

}
