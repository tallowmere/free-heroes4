#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "h4/app/controller.hpp"
#include "h4/core/clock.hpp"
#include "h4/cursor/cursor_source.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/platform/window.hpp"
#include "h4/screens/shell/help_overlay.hpp"

namespace h4::app {

class Application final {
public:
    Application(platform::Window& window,
                cursor::CursorSource& cursors,
                std::unique_ptr<Controller> initial,
                std::optional<screens::HelpOverlay> help = std::nullopt);

    [[nodiscard]] const screens::HelpOverlay* help() const noexcept {
        return help_ ? &*help_ : nullptr;
    }

    void run();

    [[nodiscard]] bool step(std::chrono::milliseconds elapsed);

private:
    [[nodiscard]] bool dispatch_input();
    void advance(std::chrono::milliseconds elapsed);
    void render();
    [[nodiscard]] bool take_transition();
    void update_cursor();
    void resize_to_window();

    platform::Window* window_;
    cursor::CursorSource* cursors_;
    std::unique_ptr<Controller> controller_;
    graphics::Surface565 frame_;
    const platform::CursorImage* shown_cursor_ = nullptr;
    core::Instant now_;
    std::optional<screens::HelpOverlay> help_;
};

}
