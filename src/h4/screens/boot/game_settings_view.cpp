#include "h4/screens/boot/game_settings_view.hpp"

#include <array>
#include <utility>

#include "h4/model/game_settings.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/label.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kHelpWindow = "options_menu";

constexpr std::array<ResolutionModel, GameSettingsModel::kResolutions> kResolutions{{
    {"800", "800_text", 800, 600, "800_600"},
    {"1024", "1024_text", 1024, 768, "1024_768"},
    {"1280", "1280_text", 1280, 1024, "1280_1024"},
}};

constexpr std::array<CheckModel, 1> kFullScreen{{{"full_screen", "full_screen", "full_screen"}}};
constexpr std::array<CheckModel, GameSettingsModel::kChecks - 1> kOptionRows{{
    {"Quick_Combat", "options_quick_combat.misc", "quick_combat"},
    {"Coordinates", "options_show_coordinates.misc", "coordinates"},
    {"Army_Reminder", "options_army_reminder.misc", "show_army_reminder"},
    {"Movement_Path", "options_movement_path.misc", "show_movement_path"},
    {"Animation", "options_adventure_animations.misc", "animation"},
}};

constexpr std::array<PortCheckModel, GameSettingsModel::kPortChecks> kPortRows{{
    {"Follow the window",
     "On: the frame follows the window -- its height picks the variant and its width extends the map. "
     "Off: the ticked resolution stands, letterboxed."},
}};

constexpr std::array<SliderModel, GameSettingsModel::kSliders> kSliders{{
    {"army_animation_speed", "Army_Speed_Text", "your_armies.options", "army_speed", model::kSpeedSteps},
    {"combat_animation_speed", "Combat_Speed_text", "combat.dialog", "combat_speed", model::kSpeedSteps},
    {"enemy_animation_speed", "Enemy_Speed_Text", "other_armies.options", "enemy_speed", model::kSpeedSteps},
    {"music_volume", "Music_Text", "music.dialog", "music_volume", model::kVolumeMax},
    {"sound_volume", "sound_text", "sound.dialog", "effects_volume", model::kVolumeMax},
}};

constexpr std::array<CaptionSpec, 3> kBanners{{
    {"Title", "options_title.misc"},
    {"resolution_text", "screen_resolution"},
    {"Options", "options_tab"},
}};

constexpr std::array<ButtonSpec, 2> kButtons{{
    {.guide = "cancel_button", .art = ButtonArt::faces, .faces = "button.cancel", .unread = "the options dialog's builder"},
    {.guide = "ok_button", .art = ButtonArt::faces, .faces = "button.ok", .unread = "the options dialog's builder"},
}};

CompositionRules settings_rules() {
    CompositionRules rules{
        .help_window      = kHelpWindow,
        .placed_by_screen = {
            "resolution_text", "full_screen_text", "Quick_Combat_Text", "Coordinates_Text", "Army_Reminder_Text", "Movement_Path_Text", "Animation_Text"
        },
    };
    for (const ResolutionModel& r : kResolutions)
        rules.placed_by_screen.push_back(r.text_guide);
    for (const SliderModel& s : kSliders)
        rules.placed_by_screen.push_back(s.text_guide);
    add_buttons(rules, kButtons);
    return rules;
}

}

core::Result<std::unique_ptr<GameSettingsView>>
GameSettingsView::build(const assets::Assets& assets, core::Size frame, GameSettingsModel model) {
    return compose_view<GameSettingsView>(assets, frame, std::move(model));
}

GameSettingsView::GameSettingsView(core::Size frame, GameSettingsModel model)
: ModalPopup{frame}, model_{std::move(model)} {
}

core::Result<void> GameSettingsView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.options", kHelpWindow, "game settings"});
    if (!opened)
        return std::unexpected{opened.error()};

    Builder dialog = opened->centred("options", HelpDecision::unread("the options dialog's builder"));
    hold_dialog(dialog.parent());
    dialog.compose(settings_rules());
    dialog.captions(kBanners, ui::kEngravedCentred);

    const auto parts = HelpDecision::from_parent();

    resolutions_ = &dialog.add<ResolutionSelector>(parts, dialog, kResolutions);
    connections_.relay(resolutions_->selection_changed, resolution_chosen);

    full_screen_ = &dialog.add<CheckList>(parts, dialog, kFullScreen);
    connections_.on(full_screen_->toggled,
                    [this](std::size_t, bool on) { check_toggled.emit(SettingsCheck::full_screen, on); });

    options_ = &dialog.add<OptionsColumn>(parts, dialog, kOptionRows, kPortRows);
    connections_.on(options_->retail().toggled,
                    [this](std::size_t row, bool on) { check_toggled.emit(static_cast<SettingsCheck>(row + 1), on); });
    connections_.on(options_->port_toggled, [this](std::size_t row, bool on) {
        port_check_toggled.emit(static_cast<SettingsPortCheck>(row), on);
    });

    sliders_ = &dialog.add<SliderGroup>(parts, dialog, kSliders);
    connections_.on(sliders_->slider_moved, [this](std::size_t i, std::int32_t position) {
        slider_moved.emit(static_cast<SettingsSlider>(i), position);
    });

    dialog.buttons(kButtons, buttons_, connections_, button_clicked);

    bind(model_);
    return dialog.finish();
}

void GameSettingsView::bind(const GameSettingsModel& model) {
    model_ = model;
    resolutions_->select(model.resolution);
    full_screen_->set_checked(0, model.checks[0]);
    for (std::size_t i = 1; i < GameSettingsModel::kChecks; ++i)
        options_->retail().set_checked(i - 1, model.checks[i]);
    for (std::size_t i = 0; i < GameSettingsModel::kSliders; ++i) {
        sliders_->set_value(i, model.slider_values[i]);
        sliders_->set_position(i, model.slider_positions[i]);
    }
    for (std::size_t i = 0; i < GameSettingsModel::kPortChecks; ++i)
        options_->set_port_checked(i, model.port_checks[i]);
}

bool GameSettingsView::resolution_checked(std::size_t index) const noexcept {
    return resolutions_->selected() == index;
}

core::Rect GameSettingsView::resolution_rect(std::size_t index) const noexcept {
    return resolutions_->toggle_rect(index);
}

bool GameSettingsView::checked(SettingsCheck check) const noexcept {
    const auto index = std::to_underlying(check);
    return index == 0 ? full_screen_->checked(0) : options_->retail().checked(index - 1);
}

core::Rect GameSettingsView::check_rect(SettingsCheck check) const noexcept {
    const auto index = std::to_underlying(check);
    return index == 0 ? full_screen_->check_rect(0) : options_->retail().check_rect(index - 1);
}

bool GameSettingsView::port_checked(SettingsPortCheck check) const noexcept {
    return options_->port_checked(std::to_underlying(check));
}

core::Rect GameSettingsView::port_check_rect(SettingsPortCheck check) const noexcept {
    return options_->port_check_rect(std::to_underlying(check));
}

std::string_view GameSettingsView::slider_value(SettingsSlider slider) const noexcept {
    return sliders_->value(std::to_underlying(slider));
}

core::Rect GameSettingsView::thumb_rect(SettingsSlider slider) const noexcept {
    return sliders_->thumb_rect(std::to_underlying(slider));
}

core::Rect GameSettingsView::slider_rect(SettingsSlider slider) const noexcept {
    return sliders_->slider_rect(std::to_underlying(slider));
}

core::Rect GameSettingsView::button_rect(SettingsButton button) const noexcept {
    return ui::shown_rect(buttons_[std::to_underlying(button)]);
}

core::Rect GameSettingsView::options_scrollbar_rect() const noexcept {
    return options_->scrollbar_rect();
}

}
