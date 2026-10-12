#include "h4/platform/sdl3/sdl_paths.hpp"

#include <memory>
#include <string>

#include "h4/platform/sdl3/sdl_handle.hpp"

namespace h4::platform::sdl3 {

core::Result<std::filesystem::path> preferences_path(std::string_view application) {
    const std::string name{application};
    const std::unique_ptr<char, void (*)(void*)> path{SDL_GetPrefPath(nullptr, name.c_str()), SDL_free};
    if (!path)
        return std::unexpected{sdl_error("SDL_GetPrefPath")};
    return std::filesystem::path{std::u8string{reinterpret_cast<const char8_t*>(path.get())}}.parent_path();
}

}
