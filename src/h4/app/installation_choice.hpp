#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include "h4/app/launch_options.hpp"
#include "h4/model/user_settings.hpp"

namespace h4::app {

using InstallationProbe = std::function<bool(const std::filesystem::path& data_dir)>;

[[nodiscard]] std::optional<std::filesystem::path>
choose_installation(const LaunchOptions& options, const model::UserSettings& settings, const InstallationProbe& holds_installation);

}
