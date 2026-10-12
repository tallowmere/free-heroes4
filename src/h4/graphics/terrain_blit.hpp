#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "h4/core/geometry.hpp"
#include "h4/graphics/color565.hpp"
#include "h4/graphics/diamond.hpp"

namespace h4::formats {
class TerrainTexture;
class TransitionEntry;
}

namespace h4::graphics {

class Canvas;

struct CornerSlopes {
    std::uint16_t left   = 0x8000;
    std::uint16_t top    = 0x8000;
    std::uint16_t right  = 0x8000;
    std::uint16_t bottom = 0x8000;
};

inline constexpr std::uint16_t kFlatSlope = 0x8000;

[[nodiscard]] std::uint16_t slope_coordinate(std::int32_t upper, std::int32_t lower) noexcept;

void fill_terrain_tile(const Canvas& canvas, core::Point anchor, CornerLifts lifts, Color565 colour);

void blit_terrain_tile(const Canvas& canvas,
                       const formats::TerrainTexture& texture,
                       std::size_t tile,
                       core::Point anchor,
                       CornerLifts lifts,
                       CornerSlopes slopes = {});

class TileStencil {
public:
    TileStencil() noexcept;

    void clear(const formats::TransitionEntry& entry) noexcept;

    [[nodiscard]] bool at(std::size_t column, std::size_t unit) const noexcept;

private:
    std::array<std::uint32_t, 64> bits_{};
};

struct TileCoverage {
    const formats::TransitionEntry* entry = nullptr;
    const TileStencil* stencil            = nullptr;

    [[nodiscard]] std::uint8_t at(std::size_t column, std::size_t unit) const noexcept;
};

void blit_transition_tile(const Canvas& canvas,
                          const formats::TerrainTexture& texture,
                          std::size_t tile,
                          core::Point anchor,
                          CornerLifts lifts,
                          CornerSlopes slopes,
                          TileCoverage coverage);

class TileOffscreen {
public:
    static constexpr std::size_t kUnits = 1024;
    static constexpr std::size_t kRows  = 64;

    TileOffscreen() noexcept;

    void render(const formats::TerrainTexture& texture, std::size_t tile, std::size_t first, std::size_t last) noexcept;

    [[nodiscard]] Color565 at(std::size_t row, std::size_t unit) const noexcept;

private:
    std::array<Color565, kUnits> units_{};
    std::array<std::uint16_t, kRows + 1> starts_{};
};

void composite_transition_tile(const Canvas& canvas,
                               const TileOffscreen& offscreen,
                               core::Point anchor,
                               CornerLifts lifts,
                               CornerSlopes slopes,
                               const formats::TransitionEntry& entry);

}
