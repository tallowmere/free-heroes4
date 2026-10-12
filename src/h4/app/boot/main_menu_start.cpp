#include <algorithm>
#include <chrono>
#include <iterator>
#include <utility>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/boot/main_menu_controller.hpp"
#include "h4/assets/assets.hpp"
#include "h4/model/scenario_resolution.hpp"
#include "h4/scene/map_assets.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/boot/loading_view.hpp"
#include "h4/screens/new_game/scenario_options_view.hpp"

namespace h4::app {

void MainMenuController::open_tutorial() {
    std::vector<std::byte> resource = assets_->game_map(model::kTutorialMap);
    auto summary                    = model::read_tutorial_summary(resource);
    if (report_.failed(summary))
        return;
    auto options = screens::ScenarioOptionsView::build(*assets_, view_->frame_size(), screens::ScenarioOptionsModel{});
    if (report_.failed(options))
        return;
    auto options_view = std::move(*options);
    auto controller = std::make_unique<ScenarioOptionsController>(*options_view, *assets_, *summary, settings_.guards_move, report_);
    tutorial_.open(
        std::move(options_view),
        std::move(controller),
        [this, &resource, &summary](screens::ScenarioOptionsView&, ScenarioOptionsController& opened, core::Connections& wiring) {
            wiring.on(opened.closed, [this] { close_tutorial(); });
            wiring.on(opened.began,
                      [this, resource = std::move(resource), summary = std::move(*summary)](const model::ScenarioSetup& chosen) mutable {
                          close_tutorial();
                          start_scenario(std::filesystem::path{model::kTutorialPath},
                                         model::ScenarioSetup::tutorial(summary, chosen.guards_move()));
                          if (load_)
                              load_->resource = std::move(resource);
                      });
        },
        {[this] { close_tutorial(); }, [this] { tutorial_.controller()->begin(); }}
    );
}

void MainMenuController::close_tutorial() {
    tutorial_.close();
}

void MainMenuController::start_scenario(std::filesystem::path file, model::ScenarioSetup setup) {
    auto loading = screens::LoadingView::build(*assets_, view_->frame_size(), screens::LoadingModel{0});
    if (report_.failed(loading))
        return;
    settings_.guards_move = setup.guards_move();
    load_.emplace(Load{.file = std::move(file), .setup = std::move(setup)});
    loading_.open(std::move(*loading), [](screens::LoadingView&, core::Connections&) {}, {[this] { close_loading(); }, {}});
    select_.forget();
}

namespace {

template <class T, class Start>
std::optional<core::Result<T>> job_value(core::Pending<core::Result<T>>& job, Start start) {
    return job.start_or_take([&] { return core::Pending<core::Result<T>>::run(start()); });
}

}

bool MainMenuController::advance_load() {
    Load& load = *load_;
    switch (load.step) {
    case Load::Step::read:
    case Load::Step::resolve: land_scenario(load); return false;
    case Load::Step::open: land_opening(load); return false;
    case Load::Step::prefetch: land_art(load); return false;
    case Load::Step::compose:
    case Load::Step::shown: break;
    }
    return true;
}

void MainMenuController::load_moved(Load::Step next, std::int32_t filled) {
    load_->step = next;
    loading_.get()->bind(screens::LoadingModel{filled});
}

void MainMenuController::load_failed(const core::Error& error) {
    report_(error.message());
    close_loading();
}

void MainMenuController::land_scenario(Load& load) {
    const bool read                     = load.step == Load::Step::read;
    const model::GameDatabase* database = database_;
    auto value                          = job_value(load.scenario_job, [&] {
        return [read,
                file     = load.file,
                resource = std::move(load.resource),
                scenario = std::move(load.scenario),
                setup    = load.setup,
                database](std::stop_token) mutable -> core::Result<model::Scenario> {
            if (!read)
                return model::resolved_scenario(std::move(*scenario), setup, database);
            return resource.empty() ? model::Scenario::load(file) : model::load_tutorial(resource);
        };
    });
    if (!value)
        return;
    if (!*value)
        return load_failed(value->error());
    load.scenario = std::move(**value);
    load_moved(read ? Load::Step::resolve : Load::Step::open, read ? 25 : 50);
}

void MainMenuController::land_opening(Load& load) {
    const model::GameDatabase* database = database_;
    auto value                          = job_value(load.opening_job, [&] {
        return [scenario = std::move(*load.scenario), setup = load.setup, database](std::stop_token) mutable {
            return open_game(std::move(scenario), std::move(setup), database);
        };
    });
    if (!value)
        return;
    if (!*value)
        return load_failed(value->error());
    load.opening = std::move(**value);
    load_moved(Load::Step::prefetch, 60);
}

void MainMenuController::land_art(Load& load) {
    auto art = load.art_job.start_or_take([&] {
        std::vector<assets::AssetRequest> wanted = screens::AdventureView::assets_needed(view_->frame_size());
        std::ranges::move(scene::map_assets(load.opening->scenario.map()), std::back_inserter(wanted));
        return assets_->prefetch(wanted);
    });
    if (!art)
        return;
    assets_->adopt(std::move(*art));
    load_moved(Load::Step::compose, 75);
}

void MainMenuController::step_loading() {
    constexpr std::chrono::milliseconds kShownFor{400};
    if (!advance_load() || !load_)
        return;
    Load& load = *load_;
    if (load.step == Load::Step::compose) {
        auto view = screens::AdventureView::build(*assets_, view_->frame_size());
        if (report_.failed(view))
            return close_loading();
        load.view = std::move(*view);
        load.step = Load::Step::shown;
        loading_.get()->bind(screens::LoadingModel{100});
        loading_shown_.restart();
        return;
    }
    if (loading_shown_.age(now_) < kShownFor)
        return;
    auto game = std::make_unique<AdventureController>(
        std::move(load.view), *assets_, std::move(*load.opening), menu_factory(), report_, database_
    );
    game->set_settings(settings_);
    game->set_settings_saver(saver_);
    request(transitions::Switch{std::move(game)});
    loading_.forget();
    load_.reset();
}

void MainMenuController::close_loading() {
    loading_.close();
    set_actions_enabled(true);
    load_.reset();
}

}
