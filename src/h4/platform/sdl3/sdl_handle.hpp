#pragma once

#include <SDL3/SDL.h>

#include <format>
#include <memory>
#include <string_view>

#include "h4/core/error.hpp"

namespace h4::platform::sdl3 {

struct SdlDestroy {
    void operator()(SDL_Window* w) const noexcept {
        SDL_DestroyWindow(w);
    }
    void operator()(SDL_Renderer* r) const noexcept {
        SDL_DestroyRenderer(r);
    }
    void operator()(SDL_Texture* t) const noexcept {
        SDL_DestroyTexture(t);
    }
    void operator()(SDL_Cursor* c) const noexcept {
        SDL_DestroyCursor(c);
    }
    void operator()(SDL_Surface* s) const noexcept {
        SDL_DestroySurface(s);
    }
};

template <class T> using SdlHandle = std::unique_ptr<T, SdlDestroy>;

[[nodiscard]] inline core::Error sdl_error(std::string_view what) {
    return core::Error{core::ErrorCode::io, std::format("{}: {}", what, SDL_GetError())};
}

}
