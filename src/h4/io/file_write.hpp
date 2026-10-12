#pragma once

#include <filesystem>
#include <string_view>

#include "h4/core/error.hpp"

namespace h4::io {

[[nodiscard]] core::Result<void> write_file_atomically(const std::filesystem::path& path, std::string_view contents);

}
