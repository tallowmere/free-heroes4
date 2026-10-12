#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "h4/platform/sdl3/sdl_inbox.hpp"

namespace h4::platform::sdl3 {

class WebSignIn final {
public:
    [[nodiscard]] static bool offered() noexcept;
    void open(SDL_Window* window, std::string_view url, std::string_view landing_prefix);

    [[nodiscard]] std::vector<std::string> take() {
        return inbox_->take();
    }

private:
    std::shared_ptr<Inbox> inbox_ = std::make_shared<Inbox>();
};

}
