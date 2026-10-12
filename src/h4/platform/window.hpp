#pragma once

#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/platform/cursor_image.hpp"
#include "h4/platform/input.hpp"

namespace h4::graphics {
class Surface565;
}

namespace h4::platform {

class Window {
public:
    virtual ~Window();

    [[nodiscard]] virtual core::Size logical_size() const noexcept = 0;

    [[nodiscard]] virtual core::Size window_size() const noexcept = 0;

    virtual void set_logical_size(core::Size size) = 0;

    [[nodiscard]] virtual std::vector<InputEvent> poll_events() = 0;

    virtual void present(const graphics::Surface565& frame) = 0;

    virtual void set_cursor(const CursorImage* image) = 0;

    virtual void open_file_dialog(std::string_view description, std::string_view extension) = 0;

    [[nodiscard]] virtual bool offers_web_sign_in() const noexcept = 0;
    virtual void open_web_sign_in(std::string_view url, std::string_view landing_prefix) = 0;

protected:
    Window()                         = default;
    Window(const Window&)            = default;
    Window& operator=(const Window&) = default;
    Window(Window&&)                 = default;
    Window& operator=(Window&&)      = default;
};

}
