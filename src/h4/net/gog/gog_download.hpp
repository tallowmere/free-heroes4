#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/net/http.hpp"

namespace h4::net::gog {

struct DownloadProgress {
    std::atomic<std::uint64_t> bytes_done{0};
    std::atomic<std::uint64_t> bytes_total{0};
};

[[nodiscard]] core::Result<std::filesystem::path> download_installer(HttpClient& http,
                                                                     const std::string& code,
                                                                     std::uint64_t product,
                                                                     std::string_view language,
                                                                     const std::filesystem::path& directory,
                                                                     DownloadProgress& progress,
                                                                     std::stop_token stop);

}
