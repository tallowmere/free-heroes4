#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

struct InstallerPart {
    std::uint32_t slice         = 0;
    std::uint64_t chunk_offset  = 0;
    std::uint64_t chunk_size    = 0;
    std::uint64_t file_offset   = 0;
    std::uint64_t stored_size   = 0;
    std::uint64_t inflated_size = 0;
    bool compressed             = false;
    bool encrypted              = false;
};

struct InstallerFile {
    std::string destination;
    std::vector<InstallerPart> parts;

    [[nodiscard]] std::uint64_t size() const noexcept;
};

inline constexpr std::string_view kHeroes4GameId = "1207658915";

class Installer final {
public:
    [[nodiscard]] static core::Result<Installer> read(std::span<const std::byte> exe);
    [[nodiscard]] static core::Result<Installer> open(const std::filesystem::path& exe);

    [[nodiscard]] const std::string& game_id() const noexcept {
        return game_id_;
    }
    [[nodiscard]] std::span<const InstallerFile> files() const noexcept {
        return files_;
    }
    [[nodiscard]] std::vector<InstallerFile> files_under(std::span<const std::string_view> prefixes) const;

private:
    Installer(std::string game_id, std::vector<InstallerFile> files)
    : game_id_{std::move(game_id)}, files_{std::move(files)} {
    }

    std::string game_id_;
    std::vector<InstallerFile> files_;
};

[[nodiscard]] std::filesystem::path installer_slice_path(const std::filesystem::path& exe, std::uint32_t slice);
[[nodiscard]] std::optional<std::filesystem::path> installer_of_slice(const std::filesystem::path& slice);

}
