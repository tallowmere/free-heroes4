#include "h4/formats/terrain_texture.hpp"

#include <array>
#include <numeric>

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

namespace {

constexpr std::array<std::uint8_t, kTerrainRows> kRowWidths{1,  1,  3,  3,  5,  5,  7,  7,  9,  9,  11, 11, 13,
                                                            13, 15, 15, 17, 17, 19, 19, 21, 21, 23, 23, 25, 25,
                                                            27, 27, 29, 29, 31, 31, 31, 31, 29, 29, 27, 27, 25,
                                                            25, 23, 23, 21, 21, 19, 19, 17, 17, 15, 15, 13, 13,
                                                            11, 11, 9,  9,  7,  7,  5,  5,  3,  3,  1,  1};

constexpr std::uint32_t kPaletteEntries = 256;

[[nodiscard]] bool row_covers(std::span<const std::uint8_t> bytes, std::size_t width) {
    std::size_t cursor = 0;
    std::size_t at     = 0;
    for (;;) {
        if (at >= bytes.size())
            return cursor == width && at == bytes.size();
        cursor += bytes[at++];
        if (cursor == width)
            return at == bytes.size();
        if (cursor > width || at >= bytes.size())
            return false;
        const std::size_t count = bytes[at++];
        if (at + count > bytes.size() || cursor + count > width)
            return false;
        at += count;
        cursor += count;
        if (cursor == width)
            return at == bytes.size();
    }
}

}

std::span<const std::uint8_t> terrain_row_widths() noexcept {
    return kRowWidths;
}

std::span<const std::uint8_t> TerrainTexture::row(std::size_t tile, std::size_t row) const noexcept {
    if (tile >= tiles_.size() || row >= kTerrainRows)
        return {};
    const Tile& t = tiles_[tile];
    return std::span{blob_}.subspan(t.begin + t.rows[row], t.rows[row + 1] - t.rows[row]);
}

bool TerrainTexture::blank(std::size_t tile) const noexcept {
    for (std::size_t r = 0; r < kTerrainRows; ++r) {
        const std::span<const std::uint8_t> bytes = row(tile, r);
        if (bytes.size() != 1)
            return false;
    }
    return true;
}

core::Result<TerrainTexture> TerrainTexture::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, "terrain texture"};
    TerrainTexture out;
    out.tiles_.resize(kTerrainTiles);
    out.blob_.reserve(payload.size());

    for (std::size_t i = 0; in.ok() && i < kTerrainTiles; ++i) {
        Tile& tile                           = out.tiles_[i];
        const std::size_t length             = in.bounded_count(in.u32(), 1);
        tile.begin                           = static_cast<std::uint32_t>(out.blob_.size());
        const std::vector<std::uint8_t> blob = in.u8_bytes(length);
        if (!in.ok())
            break;
        out.blob_.insert(out.blob_.end(), blob.begin(), blob.end());

        std::uint32_t previous = 0;
        for (std::size_t r = 0; in.ok() && r < kTerrainRows; ++r) {
            const std::uint32_t offset = in.u32();
            if (offset > length || offset < previous) {
                in.fail(core::ErrorCode::corrupt, "terrain row offset");
                break;
            }
            tile.rows[r] = offset;
            previous     = offset;
        }
        tile.rows[kTerrainRows] = static_cast<std::uint32_t>(length);
        if (in.ok() && tile.rows[0] != 0)
            in.fail(core::ErrorCode::corrupt, "terrain row 0 offset");
        for (std::size_t r = 0; in.ok() && r < kTerrainRows; ++r)
            if (!row_covers(std::span{out.blob_}.subspan(tile.begin + tile.rows[r], tile.rows[r + 1] - tile.rows[r]), kRowWidths[r]))
                in.fail(core::ErrorCode::corrupt, "terrain row does not cover its width");
    }

    const std::size_t entries = in.bounded_count(in.u32(), 3);
    if (in.ok() && entries != kPaletteEntries)
        in.fail(core::ErrorCode::corrupt, "terrain palette size");
    out.palette_.reserve(entries);
    for (std::size_t i = 0; in.ok() && i < entries; ++i)
        out.palette_.push_back(read_palette_color(in));
    return in.finish(std::move(out));
}

}
