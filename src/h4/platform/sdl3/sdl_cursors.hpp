#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "h4/core/geometry.hpp"
#include "h4/platform/cursor_image.hpp"
#include "h4/platform/sdl3/sdl_handle.hpp"

namespace h4::platform::sdl3 {

class PointerCursors final {
public:
    explicit PointerCursors(SDL_Window* window, core::Size logical) noexcept : window_{window}, logical_{logical} {
    }
    PointerCursors(const PointerCursors&)            = delete;
    PointerCursors& operator=(const PointerCursors&) = delete;
    ~PointerCursors();

    void show(const CursorImage* image);

    void rescale();

    void set_logical(core::Size logical);

private:
    void show_current();

    [[nodiscard]] float point_scale() const;

    [[nodiscard]] SdlHandle<SDL_Cursor> build(const CursorImage& image) const;

    SDL_Window* window_;
    core::Size logical_;
    std::unordered_map<std::uint32_t, SdlHandle<SDL_Cursor>> cursors_;
    std::optional<CursorImage> current_;
};

}
