#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::ui {
class Button;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

enum class MainMenuAction : std::uint8_t { new_game, load_game, options, quit, multiplayer };
inline constexpr std::size_t kMainMenuActions = 5;

struct MenuBanner {
    std::string_view faces_base;
    std::string_view text_key;
    std::string_view help_item;
};

class MenuBanners final : public Grouping {
public:
    MenuBanners(Builder& builder, std::span<const MenuBanner, kMainMenuActions> banners);

    void set_enabled_all(bool enabled);
    [[nodiscard]] core::Rect button_rect(MainMenuAction action) const noexcept;
    [[nodiscard]] core::Rect art_rect(MainMenuAction action) const noexcept;

    core::Signal<MainMenuAction> clicked;

private:
    [[nodiscard]] const ui::Button& button(MainMenuAction action) const noexcept {
        return *buttons_[std::to_underlying(action)];
    }

    std::array<ui::Button*, kMainMenuActions> buttons_{};
    core::Connections connections_;
};

}
