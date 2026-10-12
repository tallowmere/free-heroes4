#include "h4/platform/sdl3/sdl_web_sign_in.hpp"

namespace h4::platform::sdl3 {

#if !defined(SDL_PLATFORM_APPLE)

bool WebSignIn::offered() noexcept {
    return false;
}

void WebSignIn::open(SDL_Window* , std::string_view , std::string_view ) {
    inbox_->push({});
}

#endif

}
