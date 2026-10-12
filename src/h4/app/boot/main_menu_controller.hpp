#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/app/boot/game_opening.hpp"
#include "h4/app/boot/game_settings_controller.hpp"
#include "h4/app/boot/game_settings_popup.hpp"
#include "h4/app/boot/new_game_menu.hpp"
#include "h4/app/boot/options_menu.hpp"
#include "h4/app/controller.hpp"
#include "h4/app/modal_slot.hpp"
#include "h4/app/new_game/scenario_options_controller.hpp"
#include "h4/app/new_game/scenario_select_controller.hpp"
#include "h4/assets/prefetched.hpp"
#include "h4/core/clock.hpp"
#include "h4/core/pending.hpp"
#include "h4/core/signal.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/game_settings.hpp"
#include "h4/model/installation.hpp"
#include "h4/model/scenario.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/boot/loading_view.hpp"
#include "h4/screens/boot/main_menu_view.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/shell/popup_host.hpp"
#include "h4/screens/shell/popup_stack.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

class MainMenuController final : public Controller {
public:
    MainMenuController(std::unique_ptr<screens::MainMenuView> view,
                       const assets::Assets& assets,
                       model::InstalledContent content,
                       std::filesystem::path maps_dir,
                       Reporter report,
                       const model::GameDatabase* database);

    [[nodiscard]] screens::View& view() noexcept override {
        return host_;
    }
    void on_key(platform::Key key) override;
    void on_resized(core::Size frame) override;
    void advance(core::Instant now) override;

    [[nodiscard]] bool new_game_menu_open() const noexcept {
        return scroll_open(screens::MainMenuAction::new_game);
    }
    [[nodiscard]] bool options_menu_open() const noexcept {
        return scroll_open(screens::MainMenuAction::options);
    }
    [[nodiscard]] bool scenario_select_open() const noexcept {
        return select_.is_open();
    }
    [[nodiscard]] const ScenarioSelectController* scenario_select() const noexcept {
        return select_.controller();
    }

    void set_settings(model::GameSettings settings) noexcept;
    [[nodiscard]] const model::GameSettings& settings() const noexcept {
        return settings_;
    }
    [[nodiscard]] bool game_settings_open() const noexcept {
        return settings_popup_.is_open();
    }
    [[nodiscard]] const GameSettingsController* game_settings() const noexcept {
        return settings_popup_.controller();
    }
    [[nodiscard]] const screens::GameSettingsView* game_settings_view() const noexcept {
        return settings_popup_.view();
    }

    void set_settings_saver(SettingsSaver saver) {
        saver_ = std::move(saver);
    }

    [[nodiscard]] bool tutorial_open() const noexcept {
        return tutorial_.is_open();
    }
    [[nodiscard]] const ScenarioOptionsController* tutorial_options() const noexcept {
        return tutorial_.controller();
    }

    [[nodiscard]] bool loading() const noexcept {
        return loading_.is_open();
    }
    [[nodiscard]] const screens::LoadingView* loading_view() const noexcept {
        return loading_.get();
    }

private:
    void on_frame() override;
    void connect_view();
    void on_action(screens::MainMenuAction action);
    [[nodiscard]] core::Point action_anchor(screens::MainMenuAction action) const;
    void set_actions_enabled(bool enabled);
    [[nodiscard]] screens::MainMenuModel make_model(bool enabled) const;
    void open_scroll(screens::MainMenuAction button, std::span<const std::string_view> keys, std::function<void(std::size_t)> chosen);
    [[nodiscard]] bool scroll_open(screens::MainMenuAction button) const noexcept;
    void open_new_game_menu();
    void on_new_game_choice(std::size_t index);
    void open_options_menu();
    void on_options_choice(std::size_t index);
    void open_game_settings();
    void open_scenario_select();
    void close_scenario_select();
    void open_tutorial();
    void close_tutorial();
    void start_scenario(std::filesystem::path file, model::ScenarioSetup setup);
    void step_loading();
    [[nodiscard]] bool advance_load();
    void close_loading();
    [[nodiscard]] std::function<std::unique_ptr<Controller>(const model::GameSettings&)> menu_factory() const;

    std::unique_ptr<screens::MainMenuView> view_;
    screens::MainMenuModel model_;
    screens::PopupHost host_;
    const assets::Assets* assets_;
    const model::GameDatabase* database_ = nullptr;
    model::InstalledContent content_;
    std::filesystem::path maps_dir_;
    SettingsSaver saver_;
    Reporter report_;
    std::vector<NewGameEntry> new_game_entries_;
    std::vector<OptionsEntry> options_entries_;
    ModalSlot<screens::ScrollMenuView> scroll_{host_};
    std::optional<screens::MainMenuAction> scroll_button_;
    ControlledSlot<screens::PopupStack, ScenarioSelectController> select_{host_};
    core::Connection action_connection_;
    model::GameSettings settings_;
    GameSettingsPopup settings_popup_{host_};
    ModalSlot<screens::LoadingView> loading_{host_};
    ControlledSlot<screens::ScenarioOptionsView, ScenarioOptionsController> tutorial_{host_};
    struct Load {
        enum class Step : std::uint8_t { read, resolve, open, prefetch, compose, shown };
        Step step = Step::read;
        std::filesystem::path file;
        std::vector<std::byte> resource;
        model::ScenarioSetup setup;
        std::optional<model::Scenario> scenario;
        std::optional<GameOpening> opening;
        std::unique_ptr<screens::AdventureView> view;
        core::Pending<core::Result<model::Scenario>> scenario_job;
        core::Pending<core::Result<GameOpening>> opening_job;
        core::Pending<assets::Prefetched> art_job;
    };
    std::optional<Load> load_;
    core::Instant now_;
    core::Since loading_shown_;
    void land_scenario(Load& load);
    void land_opening(Load& load);
    void land_art(Load& load);
    void load_moved(Load::Step next, std::int32_t filled);
    void load_failed(const core::Error& error);
};

}
