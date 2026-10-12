#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

class MappedFile {
public:
    [[nodiscard]] static core::Result<MappedFile> open(const std::filesystem::path& path);

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;
    MappedFile(const MappedFile&)            = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    ~MappedFile();

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
        return {data_, size_};
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

private:
    MappedFile() = default;
    void release() noexcept;

    const std::byte* data_ = nullptr;
    std::size_t size_      = 0;
    bool mapped_           = false;
    std::vector<std::byte> buffer_;
};

}
