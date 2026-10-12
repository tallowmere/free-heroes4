#include "h4/app/boot/setup_controller.hpp"

#include <algorithm>
#include <format>
#include <utility>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/core/pending.hpp"
#include "h4/io/inno/installer.hpp"
#include "h4/io/inno/managed_install.hpp"
#include "h4/net/gog/gog_api.hpp"
#include "h4/net/gog/gog_download.hpp"

namespace h4::app {

namespace {

constexpr std::string_view kDownloadLanguage = "en";

[[nodiscard]] constexpr std::uint64_t product_number(std::string_view digits) {
    std::uint64_t number = 0;
    for (const char digit : digits)
        number = number * 10 + static_cast<std::uint64_t>(digit - '0');
    return number;
}
constexpr std::uint64_t kGogProduct = product_number(io::kHeroes4GameId);
static_assert(kGogProduct == 1207658915);

[[nodiscard]] bool has_extension(const std::filesystem::path& path, std::string_view extension) {
    return core::iequals(path.extension().string(), extension);
}

[[nodiscard]] std::optional<std::filesystem::path> installer_in(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> exes;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator{dir, ec})
        if (entry.is_regular_file(ec) && has_extension(entry.path(), ".exe"))
            exes.push_back(entry.path());
    std::ranges::sort(exes);
    const auto sliced = std::ranges::find_if(exes, [](const std::filesystem::path& exe) {
        std::error_code missing;
        return std::filesystem::exists(io::installer_slice_path(exe, 0), missing);
    });
    if (sliced != exes.end())
        return *sliced;
    if (!exes.empty())
        return exes.front();
    return std::nullopt;
}

[[nodiscard]] DropReading refused(const std::filesystem::path& dropped) {
    return {DropReading::Kind::refused,
            {},
            std::format("{} is neither the GOG installer nor a Heroes IV "
                        "installation.",
                        dropped.filename().string())};
}

}

DropReading read_drop(const std::filesystem::path& dropped, const InstallationProbe& holds_installation) {
    std::error_code ec;
    if (std::filesystem::is_directory(dropped, ec)) {
        if (holds_installation(dropped / "Data"))
            return {DropReading::Kind::installation, dropped / "Data", {}};
        if (holds_installation(dropped))
            return {DropReading::Kind::installation, dropped, {}};
        if (const auto exe = installer_in(dropped))
            return {DropReading::Kind::installer, *exe, {}};
        return refused(dropped);
    }
    if (const auto exe = io::installer_of_slice(dropped))
        return {DropReading::Kind::installer, *exe, {}};
    if (has_extension(dropped, ".exe"))
        return {DropReading::Kind::installer, dropped, {}};
    return refused(dropped);
}

struct SetupController::Extraction {
    std::size_t files   = 0;
    std::uint64_t bytes = 0;
    io::ExtractProgress progress;
    core::Pending<core::Result<io::InstallMarker>> job;
};

struct SetupController::Download {
    net::gog::DownloadProgress progress;
    core::Pending<core::Result<std::filesystem::path>> job;
};

SetupController::SetupController(std::unique_ptr<screens::SetupView> view, std::filesystem::path install_dir, SetupHooks hooks)
: view_{std::move(view)}, install_dir_{std::move(install_dir)}, hooks_{std::move(hooks)} {
    connections_.on(view_->browse_clicked, [this] {
        if (hooks_.browse)
            hooks_.browse();
    });
    connections_.on(view_->sign_in_clicked, [this] {
        if (hooks_.sign_in && !busy())
            hooks_.sign_in();
    });
    connections_.on(view_->quit_clicked, [this] { request(transitions::Quit{}); });
    connections_.on(view_->cancel_clicked, [this] { cancel(); });
}

SetupController::~SetupController() = default;

void SetupController::on_key(platform::Key key) {
    if (key != platform::Key::escape)
        return;
    if (busy())
        cancel();
    else
        request(transitions::Quit{});
}

void SetupController::on_file_dropped(const std::string& path) {
    if (busy())
        return;
    const std::filesystem::path dropped{std::u8string{path.begin(), path.end()}};
    const DropReading reading = read_drop(dropped, hooks_.holds_installation);
    switch (reading.kind) {
    case DropReading::Kind::installation: finish(reading.path); return;
    case DropReading::Kind::installer: start_extraction(reading.path); return;
    case DropReading::Kind::refused: view_->show_waiting(reading.message); return;
    }
}

void SetupController::on_web_sign_in(const std::string& landing) {
    if (busy() || hooks_.http == nullptr)
        return;
    auto code = net::gog::code_from_landing(landing);
    if (!code) {
        view_->show_waiting(landing.empty() ? "Sign-in cancelled." : "GOG's sign-in did not finish.");
        return;
    }
    start_download(std::move(*code));
}

void SetupController::start_download(std::string code) {
    download_      = std::make_unique<Download>();
    download_->job = core::Pending<core::Result<std::filesystem::path>>::run(
        [http = hooks_.http, code = std::move(code), directory = hooks_.download_dir, progress = &download_->progress](std::stop_token stop) {
            return net::gog::download_installer(*http, code, kGogProduct, kDownloadLanguage, directory, *progress, std::move(stop));
        }
    );
    view_->show_download(0, 0);
}

void SetupController::step_download() {
    auto done = download_->job.take();
    if (!done) {
        view_->show_download(download_->progress.bytes_done.load(), download_->progress.bytes_total.load());
        return;
    }
    download_.reset();
    if (!*done) {
        view_->show_waiting(done->error().message());
        return;
    }
    start_extraction(**done);
    from_download_ = extraction_ != nullptr;
}

void SetupController::start_extraction(const std::filesystem::path& exe) {
    auto installer = io::Installer::open(exe);
    if (!installer) {
        view_->show_waiting(installer.error().message());
        return;
    }
    if (const auto slice = io::installer_slice_path(exe, 0); !std::filesystem::exists(slice)) {
        view_->show_waiting(std::format("{} is missing: it must sit beside the installer.", slice.filename().string()));
        return;
    }
    const io::EngineSubset subset = io::engine_subset(*installer);
    extraction_                   = std::make_unique<Extraction>();
    extraction_->files            = subset.files.size();
    extraction_->bytes            = subset.bytes;
    extraction_->job              = core::Pending<core::Result<io::InstallMarker>>::run(
        [installer = std::move(*installer), exe, install = install_dir_, progress = &extraction_->progress](std::stop_token stop) {
            return io::install_from_installer(installer, exe, install, *progress, std::move(stop));
        }
    );
    view_->show_progress(0, extraction_->files, 0, extraction_->bytes);
}

void SetupController::cancel() {
    if (!busy())
        return;
    download_.reset();
    extraction_.reset();
    from_download_ = false;
    view_->show_waiting("Cancelled.");
}

void SetupController::finish(const std::filesystem::path& data_dir) {
    if (hooks_.installed)
        hooks_.installed(data_dir);
    request(transitions::Quit{});
}

void SetupController::step_extraction() {
    auto done = extraction_->job.take();
    if (!done) {
        view_->show_progress(extraction_->progress.files_done.load(),
                             extraction_->files,
                             extraction_->progress.bytes_done.load(),
                             extraction_->bytes);
        return;
    }
    extraction_.reset();
    if (!*done) {
        view_->show_waiting(done->error().message());
        return;
    }
    if (std::exchange(from_download_, false)) {
        std::error_code ignored;
        std::filesystem::remove_all(hooks_.download_dir, ignored);
    }
    finish(install_dir_ / "Data");
}

void SetupController::on_frame() {
    if (download_)
        step_download();
    if (extraction_)
        step_extraction();
}

}
