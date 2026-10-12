#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/io/archive.hpp"
#include "h4/io/resource_source.hpp"

namespace h4::io {

class ArchiveChain final : public ResourceSource {
public:
    static constexpr std::string_view kBaseArchive = "heroes4.h4r";

    [[nodiscard]] static core::Result<ArchiveChain> mount_installation(const std::filesystem::path& data_dir);

    ArchiveChain(ArchiveChain&&) noexcept            = default;
    ArchiveChain& operator=(ArchiveChain&&) noexcept = default;

    [[nodiscard]] std::span<const std::unique_ptr<Archive>> archives() const noexcept {
        return archives_;
    }

    [[nodiscard]] bool contains(std::string_view name) const override;
    [[nodiscard]] core::Result<std::vector<std::byte>> read(std::string_view name) const override;

private:
    struct Location {
        const Archive* archive;
        const ArchiveEntry* entry;
    };

    ArchiveChain() = default;
    void mount(std::unique_ptr<Archive> archive);
    [[nodiscard]] const Location* locate(std::string_view name) const;

    std::vector<std::unique_ptr<Archive>> archives_;
    core::FoldedMap<std::string> aliases_;
    core::FoldedMap<Location> payloads_;
};

}
