#include "h4/io/inno/setup_loader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <optional>

#include "h4/core/byte_reader.hpp"
#include "h4/io/compression.hpp"

namespace h4::io {

namespace {

constexpr std::uint32_t kRcData          = 10;
constexpr std::uint32_t kOffsetTableId   = 11111;
constexpr std::uint32_t kSubdirectoryBit = 0x8000'0000u;
constexpr std::size_t kOffsetTableSize   = 44;
constexpr std::size_t kSubBlockSize      = 4096;
constexpr std::size_t kBlockLimit = 64u << 20;

constexpr std::array<std::uint8_t, 12> kOffsetTableMagic{0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0xCD, 0xE6, 0xD7, 0x7B, 0x0B, 0x2A};

[[nodiscard]] std::uint32_t crc_of(std::span<const std::byte> bytes) noexcept {
    return crc32({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()});
}

struct Section {
    std::uint32_t virtual_address;
    std::uint32_t size;
    std::uint32_t raw_offset;
};

struct PeImage {
    std::vector<Section> sections;
    std::uint32_t resource_rva = 0;

    [[nodiscard]] std::optional<std::size_t> file_offset(std::uint32_t rva) const {
        for (const Section& s : sections)
            if (rva >= s.virtual_address && rva - s.virtual_address < s.size)
                return std::size_t{rva} - s.virtual_address + s.raw_offset;
        return std::nullopt;
    }
};

[[nodiscard]] core::Result<PeImage> read_pe(std::span<const std::byte> exe) {
    core::ByteReader r{exe, "PE header", 0x3C};
    r.seek(r.u32());
    if (r.u32() != 0x0000'4550u)
        r.fail(core::ErrorCode::corrupt, "no PE signature");
    r.skip(2);
    const std::uint16_t section_count = r.u16();
    r.skip(12);
    const std::uint16_t optional_size = r.u16();
    r.skip(2);
    const std::size_t optional_start = r.position();
    const std::uint16_t magic        = r.u16();
    r.seek(optional_start + (magic == 0x20B ? 112 : 96) + 2 * 8);
    PeImage image;
    image.resource_rva = r.u32();
    r.seek(optional_start + optional_size);
    for (std::size_t i = 0, n = r.bounded_count(section_count, 40); i < n; ++i) {
        r.skip(8);
        const std::uint32_t virtual_size = r.u32();
        const std::uint32_t address      = r.u32();
        const std::uint32_t raw_size     = r.u32();
        const std::uint32_t raw_offset   = r.u32();
        r.skip(16);
        image.sections.push_back({address, std::max(virtual_size, raw_size), raw_offset});
    }
    return r.finish(std::move(image), false);
}

[[nodiscard]] core::Result<std::uint32_t>
resource_entry(std::span<const std::byte> exe, std::size_t base, std::uint32_t dir, std::optional<std::uint32_t> id) {
    core::ByteReader r{exe, "PE resources", base + dir};
    r.skip(12);
    const std::size_t count = std::size_t{r.u16()} + r.u16();
    for (std::size_t i = 0, n = r.bounded_count(count, 8); i < n; ++i) {
        const std::uint32_t name   = r.u32();
        const std::uint32_t target = r.u32();
        if (!id || name == *id)
            return r.finish(std::uint32_t{target}, false);
    }
    if (!r.ok())
        return std::unexpected(*r.error());
    return core::fail(core::ErrorCode::not_found,
                      std::format("no RT_RCDATA {} resource: not an Inno "
                                  "Setup installer",
                                  kOffsetTableId));
}

[[nodiscard]] core::Result<std::span<const std::byte>> offset_table(std::span<const std::byte> exe) {
    const auto pe = read_pe(exe);
    if (!pe)
        return std::unexpected(pe.error());
    const auto base = pe->file_offset(pe->resource_rva);
    if (!base)
        return core::fail(core::ErrorCode::corrupt, "PE resources outside every section");

    std::uint32_t dir = 0;
    for (const std::optional<std::uint32_t> id :
         {std::optional{kRcData}, std::optional{kOffsetTableId}, std::optional<std::uint32_t>{}}) {
        const auto entry = resource_entry(exe, *base, dir, id);
        if (!entry)
            return std::unexpected(entry.error());
        dir = *entry & ~kSubdirectoryBit;
    }
    core::ByteReader r{exe, "PE resource leaf", *base + dir};
    const std::uint32_t rva  = r.u32();
    const std::uint32_t size = r.u32();
    if (!r.ok())
        return std::unexpected(*r.error());
    const auto offset = pe->file_offset(rva);
    const auto table  = offset ? core::checked_subspan(exe, *offset, size) : std::nullopt;
    if (!table)
        return core::fail(core::ErrorCode::corrupt, "offset table outside the exe");
    return *table;
}

}

core::Result<std::size_t> setup_data_offset(std::span<const std::byte> exe) {
    const auto table = offset_table(exe);
    if (!table)
        return std::unexpected(table.error());
    if (table->size() < kOffsetTableSize || !core::starts_with_magic(*table, kOffsetTableMagic))
        return core::fail(core::ErrorCode::unsupported, "not an Inno Setup 5.1.5+ offset table");

    core::ByteReader r{*table, "Inno offset table", kOffsetTableMagic.size()};
    r.skip(4 * 4);
    r.skip(4);
    const std::uint32_t header_offset = r.u32();
    r.skip(4);
    const std::uint32_t table_crc = r.u32();
    if (r.ok() && table_crc != crc_of(table->first(kOffsetTableSize - 4)))
        r.fail(core::ErrorCode::corrupt, "offset table CRC-32 mismatch");
    if (r.ok() && header_offset >= exe.size())
        r.fail(core::ErrorCode::corrupt, "setup.0 starts past the exe's end");
    return r.finish(std::size_t{header_offset}, false);
}

core::Result<SetupBlock> read_setup_block(std::span<const std::byte> exe, std::size_t offset) {
    core::ByteReader r{exe, "setup.0 block", offset};
    const std::uint32_t header_crc = r.u32();
    const auto header = exe.subspan(std::min(exe.size(), r.position()), std::min<std::size_t>(5, r.remaining()));
    const std::uint32_t stored_size = r.u32();
    const std::uint8_t compressed   = r.u8();
    if (r.ok() && header_crc != crc_of(header))
        r.fail(core::ErrorCode::corrupt, "block header CRC-32 mismatch");

    std::vector<std::byte> content;
    const std::size_t end = r.position() + stored_size;
    while (r.ok() && r.position() < end) {
        const std::uint32_t crc = r.u32();
        const auto chunk        = r.bytes(std::min(kSubBlockSize, end - std::min(end, r.position())));
        if (r.ok() && crc != crc_of(chunk))
            r.fail(core::ErrorCode::corrupt, "sub-block CRC-32 mismatch");
        content.insert(content.end(), chunk.begin(), chunk.end());
    }
    if (!r.ok())
        return std::unexpected(*r.error());
    if (r.position() != end)
        return core::fail(core::ErrorCode::corrupt, "block ends mid sub-block");
    if (compressed == 0)
        return SetupBlock{std::move(content), end};
    if (compressed != 1)
        return core::fail(core::ErrorCode::unsupported, std::format("block compression flag {}", compressed));
    auto decoded = decode_lzma1_raw(content, kBlockLimit);
    if (!decoded)
        return std::unexpected(decoded.error().with_context("setup.0 block"));
    return SetupBlock{std::move(*decoded), end};
}

}
