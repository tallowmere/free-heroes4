#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::io {

[[nodiscard]] core::Result<void> write_png(const std::filesystem::path& path, core::Size size, std::span<const std::uint8_t> rgba);

}
