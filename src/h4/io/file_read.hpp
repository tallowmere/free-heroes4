#pragma once

#include <filesystem>
#include <string>

#include "h4/core/error.hpp"

namespace h4::io {

[[nodiscard]] core::Result<std::string> read_text_file(const std::filesystem::path& path);

}
