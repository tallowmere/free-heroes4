#pragma once

#include <filesystem>
#include <optional>

#include "h4/core/error.hpp"
#include "h4/model/user_settings.hpp"
#include "h4/platform/window.hpp"

namespace h4::root {

[[nodiscard]] core::Result<model::LoadedSettings> load_settings(const std::filesystem::path& path);

void save_settings(const std::filesystem::path& path, const model::UserSettings& settings);

[[nodiscard]] bool holds_installation(const std::filesystem::path& data_dir);

bool import_retail_settings(model::UserSettings& settings, const std::filesystem::path& data_dir);

[[nodiscard]] std::optional<std::filesystem::path> run_setup(platform::Window&            window,
                                                             const std::filesystem::path& home);

}
