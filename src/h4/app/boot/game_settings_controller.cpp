#include "h4/app/boot/game_settings_controller.hpp"

#include <algorithm>
#include <string>

#include "h4/assets/assets.hpp"

namespace h4::app {

namespace {

constexpr std::array<std::int32_t, 3> kResolutionWidths{800, 1024, 1280};

std::int32_t speed_position(std::int32_t value) noexcept {
    return std::clamp(value / model::kSpeedStep, 0, model::kSpeedSteps);
}

}

GameSettingsController::GameSettingsController(screens::GameSettingsView& view, const assets::Assets& assets, model::GameSettings settings)
: view_{&view}, assets_{&assets}, settings_{settings} {
    connections_.on(view.resolution_chosen, [this](std::size_t index) {
        model::GameSettings next = settings_;
        next.resolution          = kResolutionWidths[std::min(index, kResolutionWidths.size() - 1)];
        apply(next);
    });
    connections_.on(view.check_toggled, [this](screens::SettingsCheck check, bool on) {
        model::GameSettings next = settings_;
        switch (check) {
        case screens::SettingsCheck::full_screen: next.full_screen = on; break;
        case screens::SettingsCheck::quick_combat: next.quick_combat = on; break;
        case screens::SettingsCheck::show_coordinates: next.show_coordinates = on; break;
        case screens::SettingsCheck::movement_reminder: next.army_movement_reminder = on; break;
        case screens::SettingsCheck::show_movement_path: next.show_move_path = on; break;
        case screens::SettingsCheck::show_animations: next.adventure_animations = on; break;
        }
        apply(next);
    });
    connections_.on(view.port_check_toggled, [this](screens::SettingsPortCheck check, bool on) {
        model::GameSettings next = settings_;
        switch (check) {
        case screens::SettingsPortCheck::follow_window: next.follow_window = on; break;
        }
        apply(next);
    });
    connections_.on(view.slider_moved, [this](screens::SettingsSlider slider, std::int32_t position) {
        model::GameSettings next = settings_;
        switch (slider) {
        case screens::SettingsSlider::army_speed: next.army_speed = position * model::kSpeedStep; break;
        case screens::SettingsSlider::combat_speed: next.combat_speed = position * model::kSpeedStep; break;
        case screens::SettingsSlider::enemy_speed: next.enemy_speed = position * model::kSpeedStep; break;
        case screens::SettingsSlider::music_volume: next.music_volume = position; break;
        case screens::SettingsSlider::effects_volume: next.sound_volume = position; break;
        }
        apply(next);
    });
    connections_.on(view.button_clicked, [this](screens::SettingsButton button) {
        if (button == screens::SettingsButton::ok)
            accept();
        else
            cancelled.emit();
    });
    view.bind(model());
}

void GameSettingsController::apply(model::GameSettings settings) {
    settings_ = settings;
    view_->bind(model());
}

void GameSettingsController::accept() {
    accepted.emit(settings_);
}

void GameSettingsController::detach() noexcept {
    view_ = nullptr;
}

screens::GameSettingsModel GameSettingsController::model() const {
    return model_for(settings_, *assets_);
}

screens::GameSettingsModel GameSettingsController::model_for(const model::GameSettings& settings, const assets::Assets& assets) {
    screens::GameSettingsModel out;
    const auto width = std::ranges::find(kResolutionWidths, settings.resolution);
    out.resolution = width == kResolutionWidths.end() ? 1 : static_cast<std::size_t>(width - kResolutionWidths.begin());
    out.checks     = {settings.full_screen,
                      settings.quick_combat,
                      settings.show_coordinates,
                      settings.army_movement_reminder,
                      settings.show_move_path,
                      settings.adventure_animations};
    const auto speed = [&assets](std::int32_t value) {
        return value <= 0 ? std::string{assets.text("speed_off.options")} : std::to_string(value / 100);
    };
    out.slider_positions = {speed_position(settings.army_speed),
                            speed_position(settings.combat_speed),
                            speed_position(settings.enemy_speed),
                            std::clamp(settings.music_volume, 0, model::kVolumeMax),
                            std::clamp(settings.sound_volume, 0, model::kVolumeMax)};
    out.slider_values    = {speed(settings.army_speed),
                            speed(settings.combat_speed),
                            speed(settings.enemy_speed),
                            std::to_string(settings.music_volume),
                            std::to_string(settings.sound_volume)};
    out.port_checks      = {settings.follow_window};
    return out;
}

}
