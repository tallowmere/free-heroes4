#include "h4/app/boot/game_settings_popup.hpp"

#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/screens/boot/game_settings_view.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::app {

FramePolicy frame_policy_for(const model::GameSettings& settings) noexcept {
    FramePolicy policy;
    policy.follow_window = settings.follow_window;
    if (const auto fixed = screens::resolution_for_width(settings.resolution))
        policy.fixed = *fixed;
    return policy;
}

core::Result<void>
GameSettingsPopup::open(const assets::Assets& assets, core::Size frame, model::GameSettings settings, GameSettingsHooks hooks) {
    settings.resolution = screens::size_of(screens::variant_for_height(frame.height)).width;

    auto view = screens::GameSettingsView::build(assets, frame, GameSettingsController::model_for(settings, assets));
    if (!view)
        return std::unexpected{view.error()};

    frame_          = frame;
    hooks_          = std::move(hooks);
    auto controller = std::make_unique<GameSettingsController>(**view, assets, settings);
    slot_.open(std::move(*view),
               std::move(controller),
               [this](screens::GameSettingsView&, GameSettingsController& opened, core::Connections& wiring) {
                   wiring.on(opened.accepted, [this](const model::GameSettings& chosen) { accepted(chosen); });
                   wiring.on(opened.cancelled, [this] { close(); });
               },
               {[this] { close(); }, [this] { slot_.controller()->accept(); }});
    return {};
}

void GameSettingsPopup::accepted(const model::GameSettings& chosen) {
    if (chosen.follow_window && chosen.resolution != frame_.width)
        hooks_.report.not_ported("Changing the screen resolution");
    if (hooks_.apply)
        hooks_.apply(chosen);
    close();
}

void GameSettingsPopup::close() {
    if (!slot_.is_open())
        return;
    slot_.close();
    if (hooks_.closed)
        hooks_.closed();
}

void GameSettingsPopup::forget() noexcept {
    slot_.forget();
}

bool GameSettingsPopup::on_key(platform::Key key) {
    return slot_.on_key(key);
}

}
