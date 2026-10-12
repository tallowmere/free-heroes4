#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

struct GridSpan {
    std::int32_t start = 0;
    std::int32_t stop  = 0;

    [[nodiscard]] std::int32_t size() const noexcept {
        return stop - start;
    }
};

[[nodiscard]] std::vector<GridSpan> tile_spans(std::uint16_t width);
[[nodiscard]] std::vector<GridSpan> vertex_spans(std::uint16_t width);
[[nodiscard]] std::size_t lattice_cells(std::span<const GridSpan> spans) noexcept;

struct TileOverlay {
    std::uint8_t terrain = 0;
    std::uint8_t variant = 0;
    std::uint16_t field  = 0;
    std::uint8_t tail    = 0;
};

struct GridTile {
    std::uint8_t terrain = 0;
    std::uint8_t variant = 0;
    std::uint8_t road    = 0;
    std::uint8_t misc    = 0;
    bool flag            = false;
    std::uint32_t fog    = 0;

    std::uint32_t overlay_a      = 0;
    std::uint32_t overlay_b      = 0;
    std::uint8_t overlay_a_count = 0;
    std::uint8_t overlay_b_count = 0;
};

[[nodiscard]] std::string_view terrain_class_name(std::uint8_t terrain) noexcept;
inline constexpr std::uint8_t kTerrainClasses = 19;

[[nodiscard]] constexpr int tile_fog(std::uint32_t fog, int viewer) noexcept {
    const int shift = viewer < 0 ? 18 : 3 * viewer;
    const int group = static_cast<int>((fog >> shift) & 7U);
    return group > 2 ? 2 : group;
}

class ScenarioGrid {
public:
    [[nodiscard]] static core::Result<ScenarioGrid>
    parse(std::span<const std::byte> payload, std::size_t offset, std::uint16_t width, std::uint8_t levels, std::uint16_t version);

    [[nodiscard]] static std::size_t plane_offset(std::size_t payload_size, std::uint16_t width, std::uint8_t levels);

    [[nodiscard]] std::uint16_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::uint8_t levels() const noexcept {
        return levels_;
    }

    [[nodiscard]] const GridTile* tile(std::int32_t x, std::int32_t y, std::int32_t level = 0) const noexcept;
    [[nodiscard]] std::uint8_t elevation(std::int32_t x, std::int32_t y, std::int32_t level = 0) const noexcept;
    [[nodiscard]] std::int32_t vertex_index(std::int32_t x, std::int32_t y, std::int32_t level = 0) const noexcept;
    [[nodiscard]] std::span<const GridSpan> vertex_columns() const noexcept {
        return vertex_spans_;
    }
    [[nodiscard]] std::span<const std::uint8_t> elevations() const noexcept {
        return elevation_;
    }

    [[nodiscard]] std::span<const GridTile> tiles() const noexcept {
        return tiles_;
    }
    [[nodiscard]] std::span<const GridSpan> columns() const noexcept {
        return tile_spans_;
    }
    [[nodiscard]] std::span<const TileOverlay> overlays_a() const noexcept {
        return overlay_a_;
    }
    [[nodiscard]] std::span<const TileOverlay> overlays_b() const noexcept {
        return overlay_b_;
    }
    [[nodiscard]] std::size_t end() const noexcept {
        return end_;
    }

private:
    ScenarioGrid() = default;

    [[nodiscard]] std::size_t column_base(std::int32_t x, std::int32_t level) const noexcept;

    std::uint16_t width_ = 0;
    std::uint8_t levels_ = 0;
    std::vector<GridSpan> tile_spans_;
    std::vector<GridSpan> vertex_spans_;
    std::vector<std::int32_t> vertex_base_;
    std::int32_t vertex_stride_ = 0;
    std::vector<std::size_t> column_;
    std::vector<GridTile> tiles_;
    std::vector<TileOverlay> overlay_a_;
    std::vector<TileOverlay> overlay_b_;
    std::vector<std::uint8_t> elevation_;
    std::size_t end_ = 0;
};

}
