#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/io/inno/installer.hpp"
#include "h4/io/inno/installer_extract.hpp"

namespace h4::io {

inline constexpr std::array<std::string_view, 2> kEngineDirectories{"Data/", "maps/"};

struct EngineSubset {
    std::vector<InstallerFile> files;
    std::uint64_t bytes = 0;
};
[[nodiscard]] EngineSubset engine_subset(const Installer& installer);

inline constexpr std::string_view kInstallMarker = ".openh4-install";

struct InstallMarker {
    std::string game_id;
    std::size_t files   = 0;
    std::uint64_t bytes = 0;
};

[[nodiscard]] core::Result<InstallMarker> install_from_installer(const Installer& installer,
                                                                 const std::filesystem::path& exe,
                                                                 const std::filesystem::path& install,
                                                                 ExtractProgress& progress,
                                                                 std::stop_token stop);

[[nodiscard]] core::Result<InstallMarker> read_install_marker(const std::filesystem::path& install);

[[nodiscard]] std::string format_install_marker(const InstallMarker& marker);
[[nodiscard]] core::Result<InstallMarker> parse_install_marker(std::string_view text);

}
