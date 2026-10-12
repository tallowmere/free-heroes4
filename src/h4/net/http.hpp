#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>

#include "h4/core/error.hpp"

namespace h4::net {

struct HttpResponse {
    int status = 0;
    std::string body;
};

class HttpClient {
public:
    virtual ~HttpClient() = default;

    [[nodiscard]] virtual core::Result<HttpResponse>
    get(const std::string& url, const std::string& bearer, std::stop_token stop) = 0;

    [[nodiscard]] virtual core::Result<void>
    download(const std::string& url, const std::filesystem::path& to, std::atomic<std::uint64_t>& bytes_done, std::stop_token stop) = 0;

protected:
    HttpClient()                             = default;
    HttpClient(const HttpClient&)            = default;
    HttpClient& operator=(const HttpClient&) = default;
};

[[nodiscard]] inline std::unexpected<core::Error> cancelled() {
    return core::fail(core::ErrorCode::io, "Cancelled.");
}

}
