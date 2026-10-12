#include "h4/app/installation_choice.hpp"

namespace h4::app {

std::optional<std::filesystem::path>
choose_installation(const LaunchOptions& options, const model::UserSettings& settings, const InstallationProbe& holds_installation) {
    if (!options.data_dir.empty())
        return options.data_dir;
    if (!settings.data_dir.empty() && holds_installation(settings.data_dir))
        return settings.data_dir;
    return std::nullopt;
}

}
