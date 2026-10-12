#include "h4/io/png_writer.hpp"

#include <array>
#include <format>
#include <fstream>
#include <string_view>
#include <vector>

#include "h4/io/compression.hpp"

namespace h4::io {

namespace {

void put_u32_be(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int shift = 24; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>(v >> shift));
}

void put_chunk(std::vector<std::uint8_t>& out, std::string_view type, std::span<const std::uint8_t> data) {
    put_u32_be(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t crc_from = out.size();
    out.insert(out.end(), type.begin(), type.end());
    out.insert(out.end(), data.begin(), data.end());
    put_u32_be(out, crc32(std::span{out}.subspan(crc_from)));
}

}

core::Result<void> write_png(const std::filesystem::path& path, core::Size size, std::span<const std::uint8_t> rgba) {
    if (size.empty())
        return core::fail(core::ErrorCode::invalid_argument, "empty image");
    const auto w = static_cast<std::size_t>(size.width);
    const auto h = static_cast<std::size_t>(size.height);
    if (rgba.size() != w * h * 4)
        return core::fail(core::ErrorCode::invalid_argument, "pixel buffer does not match the size");

    std::vector<std::uint8_t> raw;
    raw.reserve(h * (w * 4 + 1));
    for (std::size_t y = 0; y < h; ++y) {
        raw.push_back(0);
        const auto row = rgba.subspan(y * w * 4, w * 4);
        raw.insert(raw.end(), row.begin(), row.end());
    }
    auto compressed = deflate_zlib(raw);
    if (!compressed)
        return std::unexpected{compressed.error()};

    std::vector<std::uint8_t> ihdr;
    put_u32_be(ihdr, static_cast<std::uint32_t>(w));
    put_u32_be(ihdr, static_cast<std::uint32_t>(h));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});

    std::vector<std::uint8_t> file{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    put_chunk(file, "IHDR", ihdr);
    put_chunk(file, "IDAT", *compressed);
    put_chunk(file, "IEND", {});

    std::ofstream out{path, std::ios::binary};
    out.write(reinterpret_cast<const char*>(file.data()), static_cast<std::streamsize>(file.size()));
    if (!out)
        return core::fail(core::ErrorCode::io, std::format("cannot write {}", path.string()));
    return {};
}

}
