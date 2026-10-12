#include "h4/io/inno/installer_extract.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <map>

#include "h4/core/byte_reader.hpp"
#include "h4/io/compression.hpp"
#include "h4/io/mapped_file.hpp"

namespace h4::io {

namespace {

constexpr std::array<std::uint8_t, 8> kSliceMagic{'i', 'd', 's', 'k', 'a', '3', '2', 0x1A};
constexpr std::array<std::uint8_t, 4> kChunkMagic{'z', 'l', 'b', 0x1A};

class Slices {
public:
    explicit Slices(std::filesystem::path exe) : exe_{std::move(exe)} {
    }

    [[nodiscard]] core::Result<std::span<const std::byte>> get(std::uint32_t slice) {
        if (const auto it = open_.find(slice); it != open_.end())
            return it->second.bytes();
        const auto path = installer_slice_path(exe_, slice);
        auto file       = MappedFile::open(path);
        if (!file)
            return core::fail(core::ErrorCode::not_found, std::format("slice file {} is missing", path.filename().string()));
        core::ByteReader r{file->bytes(), "slice header", kSliceMagic.size()};
        const std::uint32_t size = r.u32();
        if (!core::starts_with_magic(file->bytes(), kSliceMagic) || !r.ok() || size > file->size())
            return core::fail(core::ErrorCode::corrupt, std::format("{} is not an Inno slice", path.filename().string()));
        const auto bytes = file->bytes().first(size);
        open_.emplace(slice, std::move(*file));
        return bytes;
    }

private:
    std::filesystem::path exe_;
    std::map<std::uint32_t, MappedFile> open_;
};

[[nodiscard]] core::Result<std::span<const std::byte>> stored_bytes(std::span<const std::byte> slice, const InstallerPart& part) {
    if (part.compressed || part.encrypted)
        return core::fail(core::ErrorCode::unsupported, "part in a compressed or encrypted chunk");
    const std::uint64_t magic = kChunkMagic.size();
    if (!core::fits(slice.size(), part.chunk_offset, magic + part.chunk_size) ||
        !core::fits(part.chunk_size, part.file_offset, part.stored_size))
        return core::fail(core::ErrorCode::corrupt, "part outside its chunk or slice");
    const auto chunk = slice.subspan(static_cast<std::size_t>(part.chunk_offset));
    if (!core::starts_with_magic(chunk, kChunkMagic))
        return core::fail(core::ErrorCode::corrupt, "bad chunk magic");
    return chunk.subspan(static_cast<std::size_t>(magic + part.file_offset), static_cast<std::size_t>(part.stored_size));
}

[[nodiscard]] core::Result<void> write_file(const InstallerFile& file,
                                            Slices& slices,
                                            const std::filesystem::path& destination,
                                            ExtractProgress& progress,
                                            const std::stop_token& stop) {
    const auto target = destination / std::filesystem::path{file.destination};
    std::error_code ec;
    std::filesystem::create_directories(target.parent_path(), ec);
    if (ec)
        return core::fail(core::ErrorCode::io, std::format("cannot create {}: {}", target.parent_path().string(), ec.message()));
    std::ofstream out{target, std::ios::binary | std::ios::trunc};
    if (!out)
        return core::fail(core::ErrorCode::io, std::format("cannot write {}", target.string()));

    for (const InstallerPart& part : file.parts) {
        if (stop.stop_requested())
            return core::fail(core::ErrorCode::io, "extraction cancelled");
        const auto slice = slices.get(part.slice);
        if (!slice)
            return std::unexpected(slice.error());
        const auto stored = stored_bytes(*slice, part);
        if (!stored)
            return std::unexpected(stored.error());
        const auto inflated = inflate_exact(*stored, static_cast<std::size_t>(part.inflated_size));
        if (!inflated)
            return std::unexpected(inflated.error());
        out.write(reinterpret_cast<const char*>(inflated->data()), static_cast<std::streamsize>(inflated->size()));
        if (!out)
            return core::fail(core::ErrorCode::io, std::format("cannot write {}", target.string()));
        progress.bytes_done.fetch_add(part.inflated_size, std::memory_order_relaxed);
    }
    out.close();
    if (!out)
        return core::fail(core::ErrorCode::io, std::format("cannot write {}", target.string()));
    return {};
}

}

core::Result<ExtractSummary> extract_installer_files(std::span<const InstallerFile> files,
                                                     const std::filesystem::path& exe,
                                                     const std::filesystem::path& destination,
                                                     ExtractProgress& progress,
                                                     std::stop_token stop) {
    Slices slices{exe};
    ExtractSummary summary;
    for (const InstallerFile& file : files) {
        const auto written = write_file(file, slices, destination, progress, stop);
        if (!written)
            return std::unexpected(written.error().with_context(file.destination));
        ++summary.files;
        summary.bytes += file.size();
        progress.files_done.fetch_add(1, std::memory_order_relaxed);
    }
    return summary;
}

}
