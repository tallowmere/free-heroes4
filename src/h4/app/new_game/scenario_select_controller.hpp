#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "h4/app/controller.hpp"
#include "h4/app/modal_slot.hpp"
#include "h4/app/new_game/scenario_options_controller.hpp"
#include "h4/core/error.hpp"
#include "h4/core/pending.hpp"
#include "h4/core/signal.hpp"
#include "h4/model/scenario_folder.hpp"
#include "h4/platform/input.hpp"
#include "h4/screens/new_game/scenario_details_view.hpp"
#include "h4/screens/new_game/scenario_select_view.hpp"
#include "h4/screens/shell/popup_host.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

class ScenarioSelectController {
public:
    ScenarioSelectController(screens::ScenarioSelectView& view, screens::PopupHost& dialogs, const assets::Assets& assets, Reporter report);

    ScenarioSelectController(const ScenarioSelectController&)            = delete;
    ScenarioSelectController& operator=(const ScenarioSelectController&) = delete;

    core::Signal<> closed;
    core::Signal<std::filesystem::path, model::ScenarioSetup> began;

    void set_guards_move(bool move) noexcept {
        guards_move_ = move;
    }

    [[nodiscard]] core::Result<void> open_folder(const std::filesystem::path& dir);
    [[nodiscard]] bool scanning() const noexcept {
        return scan_.started();
    }
    void on_frame();

    [[nodiscard]] const model::ScenarioFolder* folder() const noexcept {
        return folder_ ? &*folder_ : nullptr;
    }
    [[nodiscard]] const screens::ScenarioSelectView& view() const noexcept {
        return *view_;
    }

    [[nodiscard]] bool details_open() const noexcept {
        return details_.is_open();
    }
    [[nodiscard]] const screens::ScenarioDetailsView* details() const noexcept {
        return details_.get();
    }
    void close_details();

    [[nodiscard]] bool options_open() const noexcept {
        return options_.is_open();
    }
    [[nodiscard]] const ScenarioOptionsController* options() const noexcept {
        return options_.controller();
    }
    [[nodiscard]] const screens::ScenarioOptionsView* options_view() const noexcept {
        return options_.view();
    }
    void close_options();

    void detach() noexcept;

    [[nodiscard]] bool on_key(platform::Key key);

private:
    void on_button(screens::ScenarioButton button);
    void open_details();
    void open_options();
    void on_selected(std::size_t entry);
    void on_activated(std::size_t entry);
    void on_sort(screens::ScenarioColumn column);
    void show(std::size_t selected);
    void update_description();
    void list(model::ScenarioFolder scanned);

    [[nodiscard]] screens::ScenarioSelectModel make_model() const;
    [[nodiscard]] screens::ScenarioRow row_for(const model::FolderEntry& entry) const;

    screens::ScenarioSelectView* view_;
    ModalSlot<screens::ScenarioDetailsView> details_;
    screens::ScenarioColumnSet columns_ = screens::ScenarioColumnSet::retail_default();
    ControlledSlot<screens::ScenarioOptionsView, ScenarioOptionsController> options_;
    const assets::Assets* assets_;
    Reporter report_;
    std::optional<model::ScenarioFolder> folder_;
    model::ScenarioOrder order_     = model::ScenarioOrder::map_type;
    model::SortDirection direction_ = model::SortDirection::ascending;
    std::size_t selected_           = 0;
    std::string description_;
    bool next_enabled_ = false;
    core::Connections connections_;
    bool guards_move_ = true;
    core::Pending<core::Result<model::ScenarioFolder>> scan_;
};

}
