#include "h4/formats/scenario/grid.hpp"

#include <algorithm>
#include <array>

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

namespace {

constexpr std::array<std::string_view, kTerrainClasses> kTerrainNames{"water",
                                                                      "grass",
                                                                      "rough",
                                                                      "swamp",
                                                                      "volcanic",
                                                                      "snow",
                                                                      "sand",
                                                                      "dirt",
                                                                      "subterranean",
                                                                      "river",
                                                                      "lava",
                                                                      "ice",
                                                                      "special_all",
                                                                      "special_life",
                                                                      "special_order",
                                                                      "special_death",
                                                                      "special_chaos",
                                                                      "special_nature",
                                                                      "special_combat"};

constexpr std::uint8_t variants_of(std::uint8_t terrain) noexcept {
    return terrain < 9 ? 2 : 1;
}

constexpr int tile_shape(std::uint16_t version) noexcept {
    if (version >= 26)
        return 4;
    if (version >= 19)
        return 3;
    if (version >= 8)
        return 2;
    if (version >= 6)
        return 1;
    return 0;
}

constexpr std::uint16_t kMaxGridWidth = 4096;

constexpr std::size_t min_tile_bytes(int shape) noexcept {
    return shape >= 4 ? 11 : 9;
}
constexpr std::uint32_t kDefaultFog = 0x80000;

}

std::string_view terrain_class_name(std::uint8_t terrain) noexcept {
    return terrain < kTerrainNames.size() ? kTerrainNames[terrain] : std::string_view{};
}

std::vector<GridSpan> tile_spans(std::uint16_t width) {
    const std::int32_t w = width;
    std::vector<GridSpan> spans;
    spans.reserve(width);
    for (std::int32_t i = 0; i < w; ++i)
        spans.push_back({std::max({0, w / 2 - 1 - i, i - w / 2}), std::min({w, w / 2 + 1 + i, 3 * w / 2 - i})});
    return spans;
}

std::vector<GridSpan> vertex_spans(std::uint16_t width) {
    if (width == 0)
        return {};
    const std::vector<GridSpan> tiles = tile_spans(width);
    const std::size_t w               = width;
    std::vector<GridSpan> spans;
    spans.reserve(w + 1);
    spans.push_back({tiles[0].start, tiles[0].stop + 1});
    for (std::size_t i = 1; i < w; ++i)
        spans.push_back({std::min(tiles[i - 1].start, tiles[i].start), std::max(tiles[i - 1].stop, tiles[i].stop) + 1});
    spans.push_back({tiles[w - 1].start, tiles[w - 1].stop + 1});
    return spans;
}

std::size_t lattice_cells(std::span<const GridSpan> spans) noexcept {
    std::size_t n = 0;
    for (const GridSpan& span : spans)
        n += static_cast<std::size_t>(span.size());
    return n;
}

std::size_t ScenarioGrid::plane_offset(std::size_t payload_size, std::uint16_t width, std::uint8_t levels) {
    const std::size_t plane = lattice_cells(vertex_spans(width)) * levels;
    return plane <= payload_size ? payload_size - plane : payload_size;
}

std::size_t ScenarioGrid::column_base(std::int32_t x, std::int32_t level) const noexcept {
    return column_[static_cast<std::size_t>(level) * width_ + static_cast<std::size_t>(x)];
}

const GridTile* ScenarioGrid::tile(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || level < 0 || level >= levels_)
        return nullptr;
    const GridSpan& span = tile_spans_[static_cast<std::size_t>(x)];
    if (y < span.start || y >= span.stop)
        return nullptr;
    return &tiles_[column_base(x, level) + static_cast<std::size_t>(y - span.start)];
}

std::int32_t ScenarioGrid::vertex_index(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    if (x < 0 || y < 0 || x > width_ || level < 0 || level >= levels_)
        return -1;
    const GridSpan& span = vertex_spans_[static_cast<std::size_t>(x)];
    if (y < span.start || y >= span.stop)
        return -1;
    return vertex_base_[static_cast<std::size_t>(x)] + vertex_stride_ * level + y;
}

std::uint8_t ScenarioGrid::elevation(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const std::int32_t index = vertex_index(x, y, level);
    if (index < 0 || static_cast<std::size_t>(index) >= elevation_.size())
        return 0;
    return elevation_[static_cast<std::size_t>(index)];
}

core::Result<ScenarioGrid>
ScenarioGrid::parse(std::span<const std::byte> payload, std::size_t offset, std::uint16_t width, std::uint8_t levels, std::uint16_t version) {
    core::ByteReader in{payload, "scenario grid", offset};
    ScenarioGrid out;
    if (width > kMaxGridWidth)
        return core::fail(core::ErrorCode::corrupt, "scenario grid: dimension out of range");
    out.width_        = width;
    out.levels_       = levels;
    out.tile_spans_   = tile_spans(width);
    out.vertex_spans_ = vertex_spans(width);
    out.vertex_base_.reserve(out.vertex_spans_.size());
    for (const GridSpan& span : out.vertex_spans_) {
        out.vertex_base_.push_back(out.vertex_stride_ - span.start);
        out.vertex_stride_ += span.size();
    }
    if (width == 0 || levels == 0)
        return core::fail(core::ErrorCode::corrupt, "scenario grid: empty");

    const int shape         = tile_shape(version);
    const std::size_t count = lattice_cells(out.tile_spans_) * levels;
    if (in.bounded_count(count, min_tile_bytes(shape)) == 0)
        return core::fail(core::ErrorCode::corrupt, "scenario grid: too short");
    out.tiles_.reserve(count);
    out.column_.reserve(static_cast<std::size_t>(levels) * width + 1);

    for (std::int32_t level = 0; in.ok() && level < levels; ++level) {
        for (std::int32_t x = 0; in.ok() && x < width; ++x) {
            out.column_.push_back(out.tiles_.size());
            const GridSpan& span = out.tile_spans_[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; in.ok() && y < span.stop; ++y) {
                GridTile tile;
                tile.terrain = in.u8();
                tile.variant = in.u8();
                tile.road    = in.u8();
                if (in.ok() && (tile.terrain >= kTerrainClasses || tile.variant >= variants_of(tile.terrain) || tile.road > 3)) {
                    in.fail(core::ErrorCode::corrupt, "tile fields out of range");
                    break;
                }
                if (shape >= 1) {
                    tile.misc = in.u8();
                    tile.flag = in.u8() != 0;
                    if (in.ok() && tile.misc > 2) {
                        in.fail(core::ErrorCode::corrupt, "tile misc out of range");
                        break;
                    }
                }
                if (shape == 3) {
                    const std::uint16_t packed = in.u16();
                    for (int group = 0; group < 6; ++group)
                        tile.fog |= static_cast<std::uint32_t>((packed >> (2 * group)) & 3U) << (3 * group);
                } else if (shape >= 4) {
                    tile.fog = in.u32();
                } else {
                    tile.fog = kDefaultFog;
                }
                tile.overlay_a       = static_cast<std::uint32_t>(out.overlay_a_.size());
                tile.overlay_a_count = in.u8();
                for (std::uint8_t i = 0; in.ok() && i < tile.overlay_a_count; ++i) {
                    TileOverlay overlay;
                    overlay.terrain = in.u8();
                    overlay.variant = in.u8();
                    overlay.field   = in.u16();
                    overlay.tail    = in.u8();
                    if (in.ok() && (overlay.terrain >= kTerrainClasses || overlay.variant >= variants_of(overlay.terrain))) {
                        in.fail(core::ErrorCode::corrupt, "tile overlay out of range");
                        break;
                    }
                    out.overlay_a_.push_back(overlay);
                }
                tile.overlay_b       = static_cast<std::uint32_t>(out.overlay_b_.size());
                tile.overlay_b_count = in.u8();
                for (std::uint8_t i = 0; in.ok() && i < tile.overlay_b_count; ++i) {
                    TileOverlay overlay;
                    overlay.terrain = in.u8();
                    overlay.field   = in.u16();
                    overlay.tail    = in.u8();
                    if (in.ok() && overlay.terrain >= 4) {
                        in.fail(core::ErrorCode::corrupt, "road overlay kind");
                        break;
                    }
                    out.overlay_b_.push_back(overlay);
                }
                out.tiles_.push_back(tile);
            }
        }
    }
    out.column_.push_back(out.tiles_.size());
    out.end_ = in.position();

    out.elevation_ = in.u8_bytes(lattice_cells(out.vertex_spans_) * levels);
    return in.finish(std::move(out));
}

}
