#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stop_token>

#include "h4/core/error.hpp"
#include "h4/io/inno/installer.hpp"

namespace h4::io {

struct ExtractProgress {
    std::atomic<std::uint64_t> bytes_done{0};
    std::atomic<std::uint32_t> files_done{0};
};

struct ExtractSummary {
    std::size_t files   = 0;
    std::uint64_t bytes = 0;
};

[[nodiscard]] core::Result<ExtractSummary> extract_installer_files(std::span<const InstallerFile> files,
                                                                   const std::filesystem::path& exe,
                                                                   const std::filesystem::path& destination,
                                                                   ExtractProgress& progress,
                                                                   std::stop_token stop);

}
