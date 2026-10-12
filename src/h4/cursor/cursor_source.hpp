#pragma once

#include <chrono>

#include "h4/core/clock.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/platform/cursor_image.hpp"

namespace h4::cursor {

class CursorSource {
public:
    virtual ~CursorSource() = default;

    [[nodiscard]] virtual const platform::CursorImage* image(const CursorState& state) = 0;

    virtual void advance(core::Instant now) = 0;

protected:
    CursorSource()                               = default;
    CursorSource(const CursorSource&)            = default;
    CursorSource& operator=(const CursorSource&) = default;
    CursorSource(CursorSource&&)                 = default;
    CursorSource& operator=(CursorSource&&)      = default;
};

}
