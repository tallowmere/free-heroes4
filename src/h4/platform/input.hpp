#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "h4/core/geometry.hpp"

namespace h4::platform {

enum class PointerButton : std::uint8_t { primary, secondary };

enum class Key : std::uint8_t { escape, enter, space, other };

namespace events {

struct PointerMoved {
    core::Point position;
};
struct PointerPressed {
    core::Point position;
    PointerButton button = PointerButton::primary;
    bool double_click = false;
};
struct PointerReleased {
    core::Point position;
    PointerButton button = PointerButton::primary;
};
struct KeyPressed {
    Key key = Key::other;
};
struct QuitRequested {};
struct FileDropped {
    std::string path;
};
struct WebSignInFinished {
    std::string landing;
};

}

using InputEvent = std::variant<events::PointerMoved, events::PointerPressed, events::PointerReleased, events::KeyPressed, events::QuitRequested, events::FileDropped, events::WebSignInFinished>;

[[nodiscard]] inline std::optional<core::Point> pointer_position(const InputEvent& event) noexcept {
    if (const auto* moved = std::get_if<events::PointerMoved>(&event))
        return moved->position;
    if (const auto* pressed = std::get_if<events::PointerPressed>(&event))
        return pressed->position;
    if (const auto* released = std::get_if<events::PointerReleased>(&event))
        return released->position;
    return std::nullopt;
}

}
