#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::formats {

inline constexpr std::size_t kTerrainTiles = 100;
inline constexpr std::size_t kTerrainRows  = 64;
inline constexpr std::size_t kTerrainPixels = 1024;

[[nodiscard]] std::span<const std::uint8_t> terrain_row_widths() noexcept;

class TerrainTexture {
public:
    [[nodiscard]] static core::Result<TerrainTexture> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::span<const std::uint8_t> row(std::size_t tile, std::size_t row) const noexcept;
    [[nodiscard]] bool blank(std::size_t tile) const noexcept;

    [[nodiscard]] PaletteColor colour(std::uint8_t index) const noexcept {
        return index < palette_.size() ? palette_[index] : PaletteColor{};
    }
    [[nodiscard]] std::span<const PaletteColor> palette() const noexcept {
        return palette_;
    }

private:
    TerrainTexture() = default;

    struct Tile {
        std::uint32_t begin = 0;
        std::array<std::uint32_t, kTerrainRows + 1> rows{};
    };

    std::vector<std::uint8_t> blob_;
    std::vector<Tile> tiles_;
    std::vector<PaletteColor> palette_;
};

}
