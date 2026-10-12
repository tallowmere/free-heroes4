#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "h4/core/signal.hpp"
#include "h4/screens/boot/check_list.hpp"
#include "h4/screens/boot/options_column.hpp"
#include "h4/screens/boot/resolution_selector.hpp"
#include "h4/screens/boot/slider_group.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {
class Button;
}

namespace h4::screens {

enum class SettingsCheck : std::uint8_t {
    full_screen,
    quick_combat,
    show_coordinates,
    movement_reminder,
    show_movement_path,
    show_animations
};

enum class SettingsPortCheck : std::uint8_t { follow_window };

enum class SettingsSlider : std::uint8_t { army_speed, combat_speed, enemy_speed, music_volume, effects_volume };

enum class SettingsButton : std::uint8_t { cancel, ok };

struct GameSettingsModel {
    static constexpr std::size_t kResolutions = 3;
    static constexpr std::size_t kChecks      = 6;
    static constexpr std::size_t kSliders     = 5;
    static constexpr std::size_t kPortChecks  = 1;

    std::size_t resolution = 0;
    std::array<bool, kChecks> checks{};
    std::array<std::int32_t, kSliders> slider_positions{};
    std::array<std::string, kSliders> slider_values;
    std::array<bool, kPortChecks> port_checks{};
};

class GameSettingsView final : public screens::ModalPopup {
    friend ViewFactory;
    explicit GameSettingsView(core::Size frame, GameSettingsModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<GameSettingsView>>
    build(const assets::Assets& assets, core::Size frame, GameSettingsModel model);

    void bind(const GameSettingsModel& model);

    core::Signal<std::size_t> resolution_chosen;
    core::Signal<SettingsCheck, bool> check_toggled;
    core::Signal<SettingsPortCheck, bool> port_check_toggled;
    core::Signal<SettingsSlider, std::int32_t> slider_moved;
    core::Signal<SettingsButton> button_clicked;

    [[nodiscard]] bool resolution_checked(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect resolution_rect(std::size_t index) const noexcept;
    [[nodiscard]] bool checked(SettingsCheck check) const noexcept;
    [[nodiscard]] core::Rect check_rect(SettingsCheck check) const noexcept;
    [[nodiscard]] bool port_checked(SettingsPortCheck check) const noexcept;
    [[nodiscard]] core::Rect port_check_rect(SettingsPortCheck check) const noexcept;
    [[nodiscard]] std::string_view slider_value(SettingsSlider slider) const noexcept;
    [[nodiscard]] core::Rect thumb_rect(SettingsSlider slider) const noexcept;
    [[nodiscard]] core::Rect slider_rect(SettingsSlider slider) const noexcept;
    [[nodiscard]] core::Rect button_rect(SettingsButton button) const noexcept;
    [[nodiscard]] core::Rect options_scrollbar_rect() const noexcept;

private:
    core::Result<void> compose(const assets::Assets& assets);

    GameSettingsModel model_;
    ResolutionSelector* resolutions_ = nullptr;
    CheckList* full_screen_          = nullptr;
    OptionsColumn* options_          = nullptr;
    SliderGroup* sliders_            = nullptr;
    std::array<ui::Button*, 2> buttons_{};
    core::Connections connections_;
};

}
