#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "h4/platform/sdl3/sdl_inbox.hpp"

namespace h4::platform::sdl3 {

class FileDialogs final {
public:
    void open(SDL_Window* window, std::string_view description, std::string_view extension);

    [[nodiscard]] std::vector<std::string> take() {
        return inbox_->take();
    }

private:
    std::shared_ptr<Inbox> inbox_ = std::make_shared<Inbox>();
};

}
