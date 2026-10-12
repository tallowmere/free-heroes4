#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

#include "h4/core/signal.hpp"
#include "h4/model/game_settings.hpp"
#include "h4/screens/boot/game_settings_view.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

class GameSettingsController {
public:
    GameSettingsController(screens::GameSettingsView& view, const assets::Assets& assets, model::GameSettings settings);

    [[nodiscard]] static screens::GameSettingsModel
    model_for(const model::GameSettings& settings, const assets::Assets& assets);

    GameSettingsController(const GameSettingsController&)            = delete;
    GameSettingsController& operator=(const GameSettingsController&) = delete;

    core::Signal<model::GameSettings> accepted;
    core::Signal<> cancelled;

    [[nodiscard]] const model::GameSettings& settings() const noexcept {
        return settings_;
    }

    void accept();

    void detach() noexcept;

private:
    void apply(model::GameSettings settings);
    [[nodiscard]] screens::GameSettingsModel model() const;

    screens::GameSettingsView* view_;
    const assets::Assets* assets_;
    model::GameSettings settings_;
    core::Connections connections_;
};

}
