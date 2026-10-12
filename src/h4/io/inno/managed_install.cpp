#include "h4/io/inno/managed_install.hpp"

#include <format>
#include <numeric>

#include "h4/core/plain_text.hpp"
#include "h4/io/file_read.hpp"
#include "h4/io/file_write.hpp"

namespace h4::io {

namespace {

[[nodiscard]] std::filesystem::path staging_path(const std::filesystem::path& install) {
    return install.parent_path() / (install.filename().string() + "-staging");
}

[[nodiscard]] core::Result<void> fail_fs(std::string_view what, const std::filesystem::path& path, const std::error_code& ec) {
    return core::fail(core::ErrorCode::io, std::format("cannot {} {}: {}", what, path.string(), ec.message()));
}

[[nodiscard]] core::Result<void> prepare_staging(const std::filesystem::path& staging, std::uint64_t bytes) {
    std::error_code ec;
    std::filesystem::remove_all(staging, ec);
    if (ec)
        return fail_fs("remove", staging, ec);
    std::filesystem::create_directories(staging, ec);
    if (ec)
        return fail_fs("create", staging, ec);
    const auto space = std::filesystem::space(staging, ec);
    if (ec)
        return fail_fs("measure the free space at", staging, ec);
    if (space.available < bytes)
        return core::fail(core::ErrorCode::io,
                          std::format("{} MB free at {}, the game needs {} MB", space.available >> 20, staging.string(), bytes >> 20));
    return {};
}

[[nodiscard]] core::Result<void> seal(const std::filesystem::path& staging, const std::filesystem::path& install) {
    std::error_code ec;
    std::filesystem::remove_all(install, ec);
    if (ec)
        return fail_fs("remove", install, ec);
    std::filesystem::rename(staging, install, ec);
    if (ec)
        return fail_fs("move the extraction to", install, ec);
    return {};
}

[[nodiscard]] core::Result<std::uint64_t> marker_number(std::string_view key, std::string_view text) {
    if (const auto value = core::whole_number<std::uint64_t>(text))
        return *value;
    return core::fail(core::ErrorCode::corrupt, std::format("install marker: {} = '{}' is not a number", key, text));
}

}

EngineSubset engine_subset(const Installer& installer) {
    EngineSubset out{installer.files_under(kEngineDirectories), 0};
    out.bytes = std::accumulate(out.files.begin(), out.files.end(), std::uint64_t{0}, [](std::uint64_t sum, const InstallerFile& file) {
        return sum + file.size();
    });
    return out;
}

core::Result<InstallMarker> install_from_installer(const Installer& installer,
                                                   const std::filesystem::path& exe,
                                                   const std::filesystem::path& install,
                                                   ExtractProgress& progress,
                                                   std::stop_token stop) {
    const EngineSubset subset = engine_subset(installer);
    const auto staging        = staging_path(install);
    if (const auto ready = prepare_staging(staging, subset.bytes); !ready)
        return std::unexpected(ready.error());
    const auto summary = extract_installer_files(subset.files, exe, staging, progress, std::move(stop));
    if (!summary) {
        std::error_code ec;
        std::filesystem::remove_all(staging, ec);
        return std::unexpected(summary.error());
    }

    const InstallMarker marker{installer.game_id(), summary->files, summary->bytes};
    if (const auto written = write_file_atomically(staging / kInstallMarker, format_install_marker(marker)); !written)
        return std::unexpected(written.error());
    if (const auto sealed = seal(staging, install); !sealed)
        return std::unexpected(sealed.error());
    return marker;
}

core::Result<InstallMarker> read_install_marker(const std::filesystem::path& install) {
    const auto text = read_text_file(install / kInstallMarker);
    if (!text)
        return core::fail(core::ErrorCode::not_found, std::format("{} holds no finished installation", install.string()));
    return parse_install_marker(*text);
}

std::string format_install_marker(const InstallMarker& marker) {
    return std::format("game_id = {}\nfiles = {}\nbytes = {}\n", marker.game_id, marker.files, marker.bytes);
}

core::Result<InstallMarker> parse_install_marker(std::string_view text) {
    InstallMarker marker;
    bool seen[3] = {};
    while (!text.empty()) {
        const std::string_view line = core::take_line(text);
        if (line.empty())
            continue;
        const auto eq = line.find('=');
        if (eq == std::string_view::npos)
            return core::fail(core::ErrorCode::corrupt, std::format("install marker: '{}' is not key = value", line));
        const auto key   = core::trimmed(line.substr(0, eq));
        const auto value = core::trimmed(line.substr(eq + 1));
        if (key == "game_id") {
            marker.game_id = std::string{value};
            seen[0]        = true;
            continue;
        }
        const auto number = marker_number(key, value);
        if (!number)
            return std::unexpected(number.error());
        if (key == "files")
            marker.files = static_cast<std::size_t>(*number), seen[1] = true;
        else if (key == "bytes")
            marker.bytes = *number, seen[2] = true;
        else
            return core::fail(core::ErrorCode::corrupt, std::format("install marker: unknown key '{}'", key));
    }
    if (!seen[0] || !seen[1] || !seen[2])
        return core::fail(core::ErrorCode::corrupt, "install marker: a key is missing");
    return marker;
}

}
