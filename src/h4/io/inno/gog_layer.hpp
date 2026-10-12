#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/io/inno/installer.hpp"
#include "h4/io/inno/setup_entries.hpp"

namespace h4::io {

struct GogCall {
    std::string name;
    std::vector<std::string> arguments;
};

[[nodiscard]] core::Result<std::optional<GogCall>> parse_gog_call(std::string_view code);

[[nodiscard]] core::Result<std::vector<InstallerFile>> gog_files(const SetupEntries& entries);

}
