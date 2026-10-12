#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "h4/core/geometry.hpp"

namespace h4::ui {

struct Help {
    std::string balloon;
    std::string right_click;

    [[nodiscard]] bool empty() const noexcept {
        return balloon.empty() && right_click.empty();
    }
};

using HelpFunction = std::function<Help(core::Point local)>;

using RightClickAction = std::function<void(core::Point local)>;

enum class Silence : std::uint8_t { none, retail, unread };

struct HelpAt {
    std::string balloon;
    std::string right_click;
    core::Rect right_click_owner;
    std::function<void()> take_right_click;
    Silence silence = Silence::none;
    std::string_view silent;
};

}
