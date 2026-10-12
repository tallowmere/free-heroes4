#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "h4/app/adventure/adventure_moves.hpp"
#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/app/adventure/army_mover.hpp"
#include "h4/app/adventure/journey.hpp"
#include "h4/app/adventure/message_queue.hpp"
#include "h4/app/adventure/system_menu.hpp"
#include "h4/app/boot/game_opening.hpp"
#include "h4/app/boot/game_settings_popup.hpp"
#include "h4/app/controller.hpp"
#include "h4/app/modal_slot.hpp"
#include "h4/core/signal.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/game_settings.hpp"
#include "h4/model/player_colour.hpp"
#include "h4/model/resources.hpp"
#include "h4/model/scenario.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/rules/game_state.hpp"
#include "h4/scene/viewer_shroud.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/adventure/army_window_view.hpp"
#include "h4/screens/adventure/treasure_chest_dialog.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "h4/screens/shell/popup_host.hpp"
#include "h4/ui/help.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::app {

class AdventureController final : public Controller {
public:
    using MenuFactory = std::function<std::unique_ptr<Controller>(const model::GameSettings&)>;

    AdventureController(std::unique_ptr<screens::AdventureView> view,
                        const assets::Assets& assets,
                        GameOpening opening,
                        MenuFactory menu,
                        Reporter report,
                        const model::GameDatabase* database = nullptr);

    [[nodiscard]] screens::View& view() noexcept override {
        return host_;
    }
    void on_key(platform::Key key) override;
    [[nodiscard]] core::Size preferred_frame(core::Size available) const override {
        return frame_policy().follow_window ? available : screens::size_of(frame_policy().fixed);
    }
    void on_resized(core::Size frame) override;

    void set_settings(model::GameSettings settings) noexcept;
    [[nodiscard]] const model::GameSettings& settings() const noexcept {
        return settings_;
    }

    void set_settings_saver(SettingsSaver saver) {
        saver_ = std::move(saver);
    }

    [[nodiscard]] const model::Scenario& scenario() const noexcept {
        return scenario_;
    }
    [[nodiscard]] const rules::GameState* game() const noexcept {
        return game_.has_value() ? &*game_ : nullptr;
    }
    [[nodiscard]] bool walking() const noexcept {
        return journey_.walking();
    }
    [[nodiscard]] const screens::AdventureView& frame() const noexcept {
        return *view_;
    }
    [[nodiscard]] screens::AdventureView& frame() noexcept {
        return *view_;
    }
    [[nodiscard]] std::size_t pending_messages() const noexcept {
        return messages_.size();
    }
    [[nodiscard]] bool message_open() const noexcept {
        return messages_.is_open();
    }
    [[nodiscard]] const screens::BasicDialog* message() const noexcept {
        return messages_.dialog();
    }
    [[nodiscard]] model::ResourceBundle purse() const noexcept;
    [[nodiscard]] const screens::ArmyWindowView* army_window() const noexcept {
        return army_window_.get();
    }
    [[nodiscard]] const screens::TreasureChestDialog* chest() const noexcept {
        return chest_.get();
    }
    [[nodiscard]] const screens::BasicDialog* question() const noexcept {
        return question_.get();
    }
    [[nodiscard]] const model::ScenarioSetup& setup() const noexcept {
        return setup_;
    }
    [[nodiscard]] bool system_menu_open() const noexcept {
        return system_scroll_.is_open();
    }
    [[nodiscard]] const screens::ScrollMenuView* system_menu_view() const noexcept {
        return system_scroll_.get();
    }
    [[nodiscard]] bool game_settings_open() const noexcept {
        return settings_popup_.is_open();
    }
    [[nodiscard]] const screens::GameSettingsView* game_settings_view() const noexcept {
        return settings_popup_.view();
    }

private:
    void on_button(screens::AdventureButton button);
    void ask_end_turn();
    [[nodiscard]] bool armies_can_move() const;
    void end_turn();
    void begin_turn();
    void select_first();
    void close_question();
    void open_system_menu();
    void on_system_menu(std::size_t index);
    void open_game_settings();
    void seat_viewer();
    void show_map();
    void on_messages_read();
    void show_purse();
    void show_heroes();
    void present();
    void present_map();
    void on_ring(std::size_t ring);
    void on_hero_row(std::size_t row);
    void on_hero_row_double_clicked(std::size_t row);
    void on_town_row(std::size_t row);
    void centre_on_actor(const model::MapActor& actor);
    void select(const model::MapActor* army);
    void on_map_pointed(const scene::MapHover& hover);
    [[nodiscard]] const model::Shroud* seen() const noexcept;
    [[nodiscard]] ui::Help map_words_at(const scene::MapHover& hover) const;
    void on_map_clicked(const scene::MapHover& hover);
    void connect_view();
    bool commit(core::Result<rules::GameState> next);
    bool apply(const rules::Command& command) {
        return commit(game_->apply(command));
    }

    void visit_shrine(std::uint32_t object, const rules::PathField::PathStep& step);
    void start_walk();
    void on_ticked(std::uint32_t now);
    [[nodiscard]] bool on_arrived(const ArmyMover::Arrival& arrival);
    void end_walk();
    void visit(std::uint32_t object, const rules::PathField::PathStep& step);
    [[nodiscard]] bool run_tile_triggers(std::uint32_t army, const rules::MapPoint& at);
    struct ChestVisit {
        std::uint32_t army   = 0;
        std::uint32_t object = 0;
        std::int32_t cost    = 0;
    };
    void open_army_window(std::optional<std::size_t> ring);
    void close_army_window();
    [[nodiscard]] std::optional<screens::ArmyWindowModel> army_window_model(std::optional<std::size_t> ring) const;
    void turn_army_window(std::size_t row);
    void open_chest(ChestVisit visit);
    void found_artifact(std::uint32_t object, const rules::PathField::PathStep& step, const rules::Artifact& artifact);
    [[nodiscard]] core::Point above_object(const rules::PathField::PathStep& step) const;
    void close_chest(screens::TreasureChestDialog::Choice choice);
    void take_object(std::uint32_t object);
    [[nodiscard]] bool face_walker(const rules::PathField::PathStep& step);
    void settle_walk();
    void pick_up(std::uint32_t object, const rules::PathField::PathStep& step);
    void float_pickup(std::uint32_t object, const model::ResourceBundle& contents);
    [[nodiscard]] const rules::ArmyPlace* placement_of(const model::MapActor* army) const noexcept;
    [[nodiscard]] std::int32_t left_of(const model::MapActor& army) const noexcept;
    [[nodiscard]] MovementLeft movement_left() const;
    [[nodiscard]] std::int32_t spent_of(const model::MapActor* army) const noexcept;
    void sync_armies();
    void place_army(std::uint32_t actor, const rules::MapPoint& at, const scene::ArmyPose& pose);

    model::Scenario scenario_;
    std::optional<scene::ViewerShroud> shroud_;
    std::unique_ptr<screens::AdventureView> view_;
    screens::PopupHost host_;
    const assets::Assets* assets_;
    model::ScenarioSetup setup_;
    const model::GameDatabase* database_ = nullptr;
    std::optional<model::PlayerColour> viewer_;
    const model::MapActor* selected_ = nullptr;
    std::optional<std::size_t> ring_;
    struct FramePresentation {
        std::vector<screens::HeroListEntry> heroes;
        std::optional<std::size_t> hero;
        std::vector<screens::TownListEntry> towns;
        std::optional<std::size_t> town;
        std::vector<screens::ArmyRingEntry> rings;
        std::optional<std::size_t> ring;
        screens::StatEntry strip;
        std::array<std::string, screens::kAdventureResources> resources;
        std::string day;
    };
    FramePresentation shown_;
    MenuFactory menu_;
    SettingsSaver saver_;
    Reporter report_;
    std::optional<AdventureMoves> moves_;
    std::optional<rules::GameState> game_;
    std::optional<rules::PlayerId> me_;
    Journey journey_;
    std::unordered_map<std::uint32_t, rules::MapPoint> triggered_;
    std::vector<scene::PathMark> path_;
    void notice_over(const rules::PathField::PathStep& step, screens::BasicDialogContent content);
    MessageQueue messages_{host_,
                           *assets_,
                           report_,
                           [this] { return view_->frame_size(); },
                           [this] { on_messages_read(); }};
    ModalSlot<screens::BasicDialog> question_{host_};
    ModalSlot<screens::TreasureChestDialog> chest_{host_};
    ModalSlot<screens::ArmyWindowView> army_window_{host_};
    std::optional<ChestVisit> chest_visit_;
    std::vector<SystemMenuEntry> system_menu_;
    ModalSlot<screens::ScrollMenuView> system_scroll_{host_};
    model::GameSettings settings_;
    GameSettingsPopup settings_popup_{host_};
    core::Connections connections_;
};

}
