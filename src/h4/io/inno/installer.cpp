#include "h4/io/inno/installer.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <numeric>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/core/plain_text.hpp"
#include "h4/io/inno/gog_layer.hpp"
#include "h4/io/inno/setup_entries.hpp"
#include "h4/io/inno/setup_loader.hpp"
#include "h4/io/mapped_file.hpp"

namespace h4::io {

namespace {

constexpr std::string_view kSetupVersion = "Inno Setup Setup Data (5.6.2) (u)";
constexpr std::size_t kVersionField      = 64;

[[nodiscard]] core::Result<std::string> setup_version(std::span<const std::byte> exe, std::size_t offset) {
    const auto field = core::checked_subspan(exe, offset, kVersionField);
    if (!field)
        return core::fail(core::ErrorCode::corrupt, "setup.0 shorter than its version string");
    std::string version;
    for (const std::byte b : *field) {
        if (b == std::byte{0})
            break;
        version += static_cast<char>(b);
    }
    return version;
}

}

std::uint64_t InstallerFile::size() const noexcept {
    return std::accumulate(parts.begin(), parts.end(), std::uint64_t{0}, [](std::uint64_t sum, const InstallerPart& part) {
        return sum + part.inflated_size;
    });
}

core::Result<Installer> Installer::read(std::span<const std::byte> exe) {
    const auto start = setup_data_offset(exe);
    if (!start)
        return std::unexpected(start.error());
    const auto version = setup_version(exe, *start);
    if (!version)
        return std::unexpected(version.error());
    if (*version != kSetupVersion)
        return core::fail(core::ErrorCode::unsupported, std::format("installer is '{}', only '{}' is known", *version, kSetupVersion));

    const auto primary = read_setup_block(exe, *start + kVersionField);
    if (!primary)
        return std::unexpected(primary.error());
    const auto secondary = read_setup_block(exe, primary->end);
    if (!secondary)
        return std::unexpected(secondary.error());
    auto entries = read_setup_entries(primary->content, secondary->content);
    if (!entries)
        return std::unexpected(entries.error());
    if (entries->app_id != kHeroes4GameId)
        return core::fail(core::ErrorCode::unsupported,
                          std::format("installer is for GOG game {}, not "
                                      "Heroes IV ({})",
                                      entries->app_id,
                                      kHeroes4GameId));
    auto files = gog_files(*entries);
    if (!files)
        return std::unexpected(files.error());
    return Installer{std::move(entries->app_id), std::move(*files)};
}

core::Result<Installer> Installer::open(const std::filesystem::path& exe) {
    const auto file = MappedFile::open(exe);
    if (!file)
        return std::unexpected(file.error());
    auto installer = read(file->bytes());
    if (!installer)
        return std::unexpected(installer.error().with_context(exe.filename().string()));
    return installer;
}

std::vector<InstallerFile> Installer::files_under(std::span<const std::string_view> prefixes) const {
    std::vector<InstallerFile> out;
    std::ranges::copy_if(files_, std::back_inserter(out), [&](const InstallerFile& file) {
        return std::ranges::any_of(prefixes,
                                   [&](std::string_view prefix) { return core::istarts_with(file.destination, prefix); });
    });
    return out;
}

std::filesystem::path installer_slice_path(const std::filesystem::path& exe, std::uint32_t slice) {
    return exe.parent_path() / std::format("{}-{}.bin", exe.stem().string(), std::uint64_t{slice} + 1);
}

std::optional<std::filesystem::path> installer_of_slice(const std::filesystem::path& slice) {
    if (!core::iequals(slice.extension().string(), ".bin"))
        return std::nullopt;
    const std::string stem = slice.stem().string();
    const std::size_t dash = stem.rfind('-');
    if (dash == std::string::npos || dash == 0)
        return std::nullopt;
    const auto number = core::whole_number<std::uint32_t>(std::string_view{stem}.substr(dash + 1));
    if (!number || *number == 0)
        return std::nullopt;
    return slice.parent_path() / (stem.substr(0, dash) + ".exe");
}

}
