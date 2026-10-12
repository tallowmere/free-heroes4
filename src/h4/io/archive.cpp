#include "h4/io/archive.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <unordered_set>
#include <utility>

#include "h4/core/byte_reader.hpp"
#include "h4/io/compression.hpp"

namespace h4::io {

namespace {

constexpr std::array<std::uint8_t, 3> kMagic{'H', '4', 'R'};

constexpr std::size_t kHeaderSize         = 12;
constexpr std::uint32_t kSupportedVersion = 5;
constexpr std::size_t kMinRecordSize = 4 * 4 + 3 * 2 + 4;
constexpr std::uint32_t kMaxInflated = std::uint32_t{1} << 26;

std::vector<ArchiveEntry> read_directory(core::ByteReader& reader, std::uint32_t declared_count) {
    const std::size_t count = reader.bounded_count(declared_count, kMinRecordSize);
    std::vector<ArchiveEntry> entries;
    entries.reserve(count);
    for (std::size_t i = 0; i < count && reader.ok(); ++i) {
        ArchiveEntry e;
        e.offset            = reader.u32();
        e.compressed_size   = reader.u32();
        e.uncompressed_size = reader.u32();
        e.timestamp         = reader.u32();
        e.name              = std::string{reader.string16()};
        e.source_path       = std::string{reader.string16()};
        e.link_target       = std::string{reader.string16()};
        e.compression       = static_cast<Compression>(reader.u32());
        entries.push_back(std::move(e));
    }
    return entries;
}

std::optional<std::vector<ArchiveEntry>> try_directory_first(std::span<const std::byte> bytes, std::uint32_t count) {
    core::ByteReader reader{bytes, "directory (layout A)", kHeaderSize};
    auto entries = read_directory(reader, count);
    if (!reader.ok() || entries.empty())
        return std::nullopt;
    std::uint32_t lowest = std::numeric_limits<std::uint32_t>::max();
    for (const auto& e : entries)
        if (!e.is_link())
            lowest = std::min(lowest, e.offset);
    if (lowest != reader.position())
        return std::nullopt;
    return entries;
}

std::optional<std::vector<ArchiveEntry>> try_directory_last(std::span<const std::byte> bytes, std::uint32_t directory_offset) {
    core::ByteReader reader{bytes, "directory (layout B)", directory_offset};
    const std::uint32_t count = reader.u32();
    auto entries              = read_directory(reader, count);
    if (!reader.ok() || entries.empty())
        return std::nullopt;
    return entries;
}

}

Archive::Archive(std::filesystem::path path, MappedFile file, std::vector<ArchiveEntry> entries)
: path_{std::move(path)}, file_{std::move(file)}, entries_{std::move(entries)} {
    index_.reserve(entries_.size());
    for (std::size_t i = 0; i < entries_.size(); ++i)
        index_.try_emplace(entries_[i].name, i);
}

core::Result<Archive> Archive::open(const std::filesystem::path& path) {
    auto file = MappedFile::open(path);
    if (!file)
        return std::unexpected{file.error()};
    const auto bytes = file->bytes();

    core::ByteReader header{bytes, "archive header"};
    const auto magic   = header.bytes(3);
    const auto version = header.u8();
    const auto word1   = header.u32();
    const auto word2   = header.u32();
    if (!header.ok())
        return core::fail(core::ErrorCode::corrupt, std::format("{}: shorter than a header", path.string()));
    if (!core::starts_with_magic(magic, kMagic))
        return core::fail(core::ErrorCode::corrupt, std::format("{}: not an H4R archive", path.string()));
    if (version != kSupportedVersion)
        return core::fail(core::ErrorCode::unsupported, std::format("{}: archive version {}", path.string(), version));

    auto entries = try_directory_first(bytes, word2);
    if (!entries)
        entries = try_directory_last(bytes, word1);
    if (!entries)
        return core::fail(core::ErrorCode::corrupt, std::format("{}: no valid directory", path.string()));

    for (const auto& e : *entries) {
        if (e.is_link())
            continue;
        if (!core::fits(bytes.size(), e.offset, e.compressed_size))
            return core::fail(core::ErrorCode::corrupt,
                              std::format("{}: {} runs past the end of the "
                                          "file",
                                          path.string(),
                                          e.name));
        if (e.compression == Compression::gzip && e.uncompressed_size > kMaxInflated)
            return core::fail(core::ErrorCode::corrupt,
                              std::format("{}: {} declares {} inflated bytes", path.string(), e.name, e.uncompressed_size));
    }
    return Archive{path, std::move(*file), std::move(*entries)};
}

const ArchiveEntry* Archive::entry(std::string_view name) const {
    const auto it = index_.find(name);
    return it == index_.end() ? nullptr : &entries_[it->second];
}

const ArchiveEntry* Archive::resolve(std::string_view name) const {
    core::FoldedSet seen;
    const ArchiveEntry* e = entry(name);
    while (e != nullptr && e->is_link()) {
        if (!seen.insert(e->name).second)
            return nullptr;
        e = entry(e->link_target);
    }
    return e;
}

core::Result<std::vector<std::byte>> Archive::read_entry(const ArchiveEntry& e) const {
    if (e.is_link())
        return core::fail(core::ErrorCode::invalid_argument, std::format("{} is a link and owns no payload", e.name));
    const auto raw = file_.bytes().subspan(e.offset, e.compressed_size);
    switch (e.compression) {
    case Compression::stored:
        if (e.compressed_size != e.uncompressed_size)
            return core::fail(core::ErrorCode::corrupt, std::format("{}: stored sizes disagree", e.name));
        return std::vector<std::byte>{raw.begin(), raw.end()};
    case Compression::gzip: {
        auto out = inflate_exact(raw, e.uncompressed_size);
        if (!out)
            return std::unexpected{out.error().with_context(e.name)};
        return out;
    }
    }
    return core::fail(core::ErrorCode::unsupported,
                      std::format("{}: compression tag {}", e.name, std::to_underlying(e.compression)));
}

}
