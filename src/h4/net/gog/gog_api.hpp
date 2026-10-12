#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::net::gog {

inline constexpr std::string_view kLandingPrefix = "https://embed.gog.com/on_login_success";
inline constexpr std::string_view kOwnedUrl      = "https://embed.gog.com/user/data/games";

[[nodiscard]] std::string login_url();
[[nodiscard]] std::optional<std::string> code_from_landing(std::string_view url);
[[nodiscard]] std::string token_url(std::string_view code);
[[nodiscard]] core::Result<std::string> parse_access_token(int status, std::string_view body);
[[nodiscard]] core::Result<bool> parse_owns(std::string_view body, std::uint64_t product);

[[nodiscard]] std::string product_url(std::uint64_t product);

struct FileLink {
    std::string id;
    std::uint64_t size = 0;
    std::string downlink;
};

struct Installer {
    std::string id;
    std::string os;
    std::string language;
    std::string version;
    std::vector<FileLink> files;
};

[[nodiscard]] core::Result<std::vector<Installer>> parse_installers(std::string_view body);
[[nodiscard]] std::optional<Installer> choose_installer(std::span<const Installer> installers, std::string_view language);
[[nodiscard]] core::Result<std::string> parse_downlink(std::string_view body);

[[nodiscard]] std::string exe_name_from(std::string_view cdn_url);
[[nodiscard]] std::string file_name(std::string_view exe_name, std::size_t index);

}
