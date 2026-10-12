#pragma once

#include <chrono>

#include "h4/core/clock.hpp"
#include "h4/core/geometry.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/platform/input.hpp"
#include "h4/ui/help.hpp"

namespace h4::graphics {
class Canvas;
}

namespace h4::screens {

class View {
public:
    virtual ~View() = default;

    virtual void handle(const platform::InputEvent& event)   = 0;
    virtual void paint(const graphics::Canvas& canvas) const = 0;

    virtual void advance(core::Instant ) {
    }

    [[nodiscard]] virtual cursor::CursorState cursor_state() const {
        return {};
    }

    [[nodiscard]] virtual ui::HelpAt help_at(core::Point ) const {
        return {};
    }

protected:
    View()                       = default;
    View(const View&)            = default;
    View& operator=(const View&) = default;
    View(View&&)                 = default;
    View& operator=(View&&)      = default;
};

}
