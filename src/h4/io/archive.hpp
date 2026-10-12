#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/core/error.hpp"
#include "h4/io/mapped_file.hpp"

namespace h4::io {

enum class Compression : std::uint32_t {
    stored = 1,
    gzip   = 3,
};

struct ArchiveEntry {
    std::string name;
    std::string source_path;
    std::string link_target;
    std::uint32_t offset            = 0;
    std::uint32_t compressed_size   = 0;
    std::uint32_t uncompressed_size = 0;
    std::uint32_t timestamp         = 0;
    Compression compression         = Compression::gzip;

    [[nodiscard]] bool is_link() const noexcept {
        return !link_target.empty();
    }
};

class Archive final {
public:
    [[nodiscard]] static core::Result<Archive> open(const std::filesystem::path& path);

    Archive(Archive&&) noexcept            = default;
    Archive& operator=(Archive&&) noexcept = default;

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }
    [[nodiscard]] std::span<const ArchiveEntry> entries() const noexcept {
        return entries_;
    }

    [[nodiscard]] const ArchiveEntry* entry(std::string_view name) const;

    [[nodiscard]] const ArchiveEntry* resolve(std::string_view name) const;

    [[nodiscard]] core::Result<std::vector<std::byte>> read_entry(const ArchiveEntry& entry) const;

private:
    Archive(std::filesystem::path path, MappedFile file, std::vector<ArchiveEntry> entries);

    std::filesystem::path path_;
    MappedFile file_;
    std::vector<ArchiveEntry> entries_;
    core::FoldedMap<std::size_t> index_;
};

}
