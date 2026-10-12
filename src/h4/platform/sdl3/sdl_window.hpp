#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/platform/window.hpp"

namespace h4::platform::sdl3 {

[[nodiscard]] core::Result<std::unique_ptr<Window>>
create_window(std::string_view title, core::Size logical_size, std::span<const std::byte> icon_png = {});

}
