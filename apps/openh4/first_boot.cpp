#include "first_boot.hpp"

#include <chrono>
#include <memory>
#include <print>
#include <string>

#include "h4/app/application.hpp"
#include "h4/app/boot/setup_controller.hpp"
#include "h4/app/launch_options.hpp"
#include "h4/cursor/cursor_source.hpp"
#include "h4/io/archive_chain.hpp"
#include "h4/io/inno/managed_install.hpp"
#include "h4/net/gog/gog_api.hpp"
#include "h4/net/http.hpp"
#include "h4/screens/boot/setup_view.hpp"

#if defined(__APPLE__)
#include <TargetConditionals.h>

#include "h4/net/apple/apple_http.hpp"
#endif

namespace h4::root {

namespace {

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
constexpr screens::SetupPrompt kPrompt = screens::SetupPrompt::pick_folder;
#else
constexpr screens::SetupPrompt kPrompt = screens::SetupPrompt::drop;
#endif

[[nodiscard]] std::unique_ptr<net::HttpClient> make_http_client() {
#if defined(__APPLE__)
    return net::apple::make_http_client();
#else
    return nullptr;
#endif
}

class HostPointer final : public cursor::CursorSource {
public:
    const platform::CursorImage* image(const cursor::CursorState& ) override {
        return nullptr;
    }
    void advance(h4::core::Instant ) override {
    }
};

}

core::Result<model::LoadedSettings> load_settings(const std::filesystem::path& path) {
    auto loaded = model::load_user_settings(path);
    if (loaded)
        for (const std::string& warning : loaded->warnings)
            std::println(stderr, "openh4: {}", warning);
    return loaded;
}

void save_settings(const std::filesystem::path& path, const model::UserSettings& settings) {
    if (const auto saved = model::save_user_settings(path, settings); !saved)
        std::println(stderr, "openh4: saving the settings: {}", saved.error().message());
}

bool holds_installation(const std::filesystem::path& data_dir) {
    return io::read_install_marker(data_dir.parent_path()).has_value() ||
        io::ArchiveChain::mount_installation(data_dir).has_value();
}

bool import_retail_settings(model::UserSettings& settings, const std::filesystem::path& data_dir) {
    const auto retail = app::maps_dir_of(data_dir).parent_path() / "config.ini";
    std::error_code ec;
    if (!std::filesystem::exists(retail, ec))
        return false;
    settings.game = model::GameSettings::load(retail);
    return true;
}

std::optional<std::filesystem::path> run_setup(platform::Window& window, const std::filesystem::path& home) {
    std::optional<std::filesystem::path> chosen;
    const std::unique_ptr<net::HttpClient> http = make_http_client();
    const bool offer_sign_in                    = http != nullptr && window.offers_web_sign_in();

    app::SetupHooks hooks{holds_installation,
                          [&window] { window.open_file_dialog("Heroes IV installer from GOG", "exe"); },
                          [&chosen](const std::filesystem::path& data_dir) { chosen = data_dir; },
                          {},
                          http.get(),
                          home / "download"};
    if (offer_sign_in)
        hooks.sign_in = [&window] { window.open_web_sign_in(net::gog::login_url(), net::gog::kLandingPrefix); };
    auto view = screens::SetupView::build(kPrompt, offer_sign_in);
    if (!view)
        return std::nullopt;
    auto controller = std::make_unique<app::SetupController>(std::move(*view), home / "install", std::move(hooks));
    HostPointer pointer;
    app::Application setup{window, pointer, std::move(controller)};
    setup.run();
    return chosen;
}

}
