#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "h4/app/boot/game_settings_controller.hpp"
#include "h4/app/controller.hpp"
#include "h4/app/modal_slot.hpp"
#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/model/game_settings.hpp"
#include "h4/platform/input.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::screens {
class GameSettingsView;
}

namespace h4::app {

[[nodiscard]] FramePolicy frame_policy_for(const model::GameSettings& settings) noexcept;

struct GameSettingsHooks {
    Reporter report;
    std::function<void(const model::GameSettings&)> apply;
    std::function<void()> closed;
};

class GameSettingsPopup {
public:
    explicit GameSettingsPopup(screens::PopupHost& host) noexcept : slot_{host} {
    }
    GameSettingsPopup(const GameSettingsPopup&)            = delete;
    GameSettingsPopup& operator=(const GameSettingsPopup&) = delete;

    [[nodiscard]] core::Result<void>
    open(const assets::Assets& assets, core::Size frame, model::GameSettings settings, GameSettingsHooks hooks);
    void close();
    void forget() noexcept;
    bool on_key(platform::Key key);

    [[nodiscard]] bool is_open() const noexcept {
        return slot_.is_open();
    }
    [[nodiscard]] const GameSettingsController* controller() const noexcept {
        return slot_.controller();
    }
    [[nodiscard]] const screens::GameSettingsView* view() const noexcept {
        return slot_.view();
    }

private:
    void accepted(const model::GameSettings& chosen);

    core::Size frame_;
    GameSettingsHooks hooks_;
    ControlledSlot<screens::GameSettingsView, GameSettingsController> slot_;
};

}
