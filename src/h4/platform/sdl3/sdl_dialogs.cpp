#include "h4/platform/sdl3/sdl_dialogs.hpp"

namespace h4::platform::sdl3 {

#if !defined(SDL_PLATFORM_IOS)

namespace {

struct Request {
    std::shared_ptr<Inbox> inbox;
    std::string description;
    std::string extension;
    SDL_DialogFileFilter filter{};
};

void on_picked(void* userdata, const char* const* files, int ) {
    const std::unique_ptr<Request> request{static_cast<Request*>(userdata)};
    if (files == nullptr)
        return;
    for (const char* const* file = files; *file != nullptr; ++file)
        request->inbox->push(*file);
}

}

void FileDialogs::open(SDL_Window* window, std::string_view description, std::string_view extension) {
    auto request                       = std::make_unique<Request>();
    request->inbox                     = inbox_;
    request->description               = description;
    request->extension                 = extension;
    request->filter                    = {request->description.c_str(), request->extension.c_str()};
    const SDL_DialogFileFilter* filter = &request->filter;
    SDL_ShowOpenFileDialog(on_picked, request.release(), window, filter, 1, nullptr, false);
}

#endif

}
