#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::app {

struct LaunchOptions {
    std::filesystem::path data_dir;
    std::optional<screens::Resolution> resolution;
    bool show_help = false;

    [[nodiscard]] static core::Result<LaunchOptions> parse(std::span<const std::string_view> args, const char* env_data_dir);

    [[nodiscard]] static std::string_view usage() noexcept;
};

[[nodiscard]] std::filesystem::path maps_dir_of(const std::filesystem::path& data_dir);

}
