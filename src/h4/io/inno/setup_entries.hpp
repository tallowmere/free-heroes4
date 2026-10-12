#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

struct SetupFileEntry {
    std::string destination;
    std::string after_install;
    std::string before_install;
    std::uint32_t location = 0;
};

inline constexpr std::uint32_t kNoLocation = 0xFFFF'FFFFu;

struct SetupDataEntry {
    std::uint32_t first_slice  = 0;
    std::uint32_t last_slice   = 0;
    std::uint32_t chunk_offset = 0;
    std::uint64_t file_offset  = 0;
    std::uint64_t file_size    = 0;
    std::uint64_t chunk_size   = 0;
    std::uint16_t flags        = 0;

    [[nodiscard]] bool encrypted() const noexcept {
        return (flags & (1u << 6)) != 0;
    }
    [[nodiscard]] bool compressed() const noexcept {
        return (flags & (1u << 7)) != 0;
    }
};

struct SetupEntries {
    std::string app_id;
    std::vector<SetupFileEntry> files;
    std::vector<SetupDataEntry> data;
};

[[nodiscard]] core::Result<SetupEntries> read_setup_entries(std::span<const std::byte> primary, std::span<const std::byte> secondary);

}
