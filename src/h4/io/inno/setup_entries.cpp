#include "h4/io/inno/setup_entries.hpp"

#include <array>
#include <utility>

#include "h4/core/byte_reader.hpp"

namespace h4::io {

namespace {

constexpr std::size_t kVersionRange = 20;
constexpr std::size_t kHeaderFixed  = 93;
constexpr std::size_t kDataEntry    = 74;

enum class List : std::uint8_t {
    languages,
    messages,
    permissions,
    types,
    components,
    tasks,
    directories,
    files,
    data_entries,
};

constexpr std::size_t kCountTotal = 16;

using Counts = std::array<std::uint32_t, kCountTotal>;

[[nodiscard]] std::uint32_t count_of(const Counts& counts, List list) noexcept {
    return counts[std::to_underlying(list)];
}

[[nodiscard]] std::span<const std::byte> string_bytes(core::ByteReader& r) {
    return r.bytes(r.u32());
}

void skip_strings(core::ByteReader& r, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
        static_cast<void>(string_bytes(r));
}

void append_utf8(std::string& out, char32_t c) {
    if (c < 0x80) {
        out += static_cast<char>(c);
        return;
    }
    if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
    } else {
        if (c < 0x10000) {
            out += static_cast<char>(0xE0 | (c >> 12));
        } else {
            out += static_cast<char>(0xF0 | (c >> 18));
            out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        }
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
    }
    out += static_cast<char>(0x80 | (c & 0x3F));
}

[[nodiscard]] std::string wide_string(core::ByteReader& r) {
    const auto bytes = string_bytes(r);
    if (bytes.size() % 2 != 0)
        r.fail(core::ErrorCode::corrupt, "odd-length UTF-16 string");
    std::string out;
    for (std::size_t i = 0; r.ok() && i + 1 < bytes.size(); i += 2) {
        char32_t unit = core::le_u16(bytes.subspan(i).first<2>());
        if (unit >= 0xD800 && unit < 0xDC00 && i + 3 < bytes.size()) {
            const char32_t low = core::le_u16(bytes.subspan(i + 2).first<2>());
            if (low >= 0xDC00 && low < 0xE000) {
                unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            }
        }
        if (unit >= 0xD800 && unit < 0xE000)
            r.fail(core::ErrorCode::corrupt, "unpaired surrogate");
        append_utf8(out, unit);
    }
    return out;
}

void read_header(core::ByteReader& r, std::string& app_id, Counts& counts) {
    skip_strings(r, 2);
    app_id = wide_string(r);
    skip_strings(r, 27 + 4);
    for (std::uint32_t& count : counts)
        count = r.u32();
    r.skip(kHeaderFixed);
}

void skip_lists(core::ByteReader& r, const Counts& counts) {
    struct Shape {
        List list;
        std::size_t strings;
        std::size_t fixed;
    };
    constexpr std::array<Shape, 7> kShapes{{
        {List::languages, 10, 5 * 4 + 1},
        {List::messages, 2, 4},
        {List::permissions, 1, 0},
        {List::types, 4, kVersionRange + 1 + 1 + 8},
        {List::components, 5, 8 + 4 + 1 + kVersionRange + 1 + 8},
        {List::tasks, 6, 4 + 1 + kVersionRange + 1},
        {List::directories, 7, 4 + kVersionRange + 2 + 1},
    }};
    for (const Shape& shape : kShapes) {
        const std::size_t n = r.bounded_count(count_of(counts, shape.list), 4 * shape.strings + shape.fixed);
        for (std::size_t i = 0; i < n; ++i) {
            skip_strings(r, shape.strings);
            r.skip(shape.fixed);
        }
    }
}

[[nodiscard]] SetupFileEntry read_file(core::ByteReader& r) {
    SetupFileEntry entry;
    skip_strings(r, 1);
    entry.destination = wide_string(r);
    skip_strings(r, 2 + 4);
    entry.after_install  = wide_string(r);
    entry.before_install = wide_string(r);
    r.skip(kVersionRange);
    entry.location = r.u32();
    r.skip(4 + 8 + 2 + 4 + 1);
    return entry;
}

[[nodiscard]] SetupDataEntry read_data(core::ByteReader& r) {
    SetupDataEntry entry;
    entry.first_slice  = r.u32();
    entry.last_slice   = r.u32();
    entry.chunk_offset = r.u32();
    entry.file_offset  = r.u64();
    entry.file_size    = r.u64();
    entry.chunk_size   = r.u64();
    r.skip(20 + 8 + 4 + 4);
    entry.flags = r.u16();
    return entry;
}

}

core::Result<SetupEntries> read_setup_entries(std::span<const std::byte> primary, std::span<const std::byte> secondary) {
    SetupEntries out;
    Counts counts{};
    core::ByteReader r{primary, "setup.0 block 1"};
    read_header(r, out.app_id, counts);
    skip_lists(r, counts);
    const std::size_t file_count = r.bounded_count(count_of(counts, List::files), 10 * 4 + kVersionRange + 23);
    out.files.reserve(file_count);
    for (std::size_t i = 0; i < file_count; ++i)
        out.files.push_back(read_file(r));
    if (!r.ok())
        return std::unexpected(*r.error());

    core::ByteReader d{secondary, "setup.0 block 2"};
    const std::size_t data_count = d.bounded_count(count_of(counts, List::data_entries), kDataEntry);
    out.data.reserve(data_count);
    for (std::size_t i = 0; i < data_count; ++i)
        out.data.push_back(read_data(d));
    return d.finish(std::move(out));
}

}
