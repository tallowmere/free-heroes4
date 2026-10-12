#pragma once

#include <filesystem>
#include <string_view>

#include "h4/core/error.hpp"

namespace h4::platform::sdl3 {

[[nodiscard]] core::Result<std::filesystem::path> preferences_path(std::string_view application);

}
