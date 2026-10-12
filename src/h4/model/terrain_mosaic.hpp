#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "h4/formats/scenario/grid.hpp"

namespace h4::model {

struct TerrainPaint {
    std::int32_t block_x = 0;
    std::int32_t block_y = 0;
    std::uint8_t slot    = 0;
};

[[nodiscard]] std::uint8_t terrain_payload_count(std::uint8_t terrain, std::uint8_t variant) noexcept;
[[nodiscard]] std::string terrain_payload_name(std::uint8_t terrain, std::uint8_t variant, std::uint8_t sub);
[[nodiscard]] std::string road_payload_name(std::uint8_t road, std::uint8_t sub);

class TerrainMosaic {
public:
    static constexpr std::int32_t kBlock = 4;

    [[nodiscard]] static TerrainMosaic build(const formats::ScenarioGrid& grid, std::uint32_t seed);

    [[nodiscard]] std::span<const TerrainPaint> paints(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;
    [[nodiscard]] std::uint8_t
    sub_variant(std::int32_t block_x, std::int32_t block_y, std::int32_t level, std::uint8_t terrain, std::uint8_t variant) const noexcept;
    [[nodiscard]] std::uint8_t
    road_sub_variant(std::int32_t block_x, std::int32_t block_y, std::int32_t level, std::uint8_t road) const noexcept;
    [[nodiscard]] std::uint8_t footprint(std::int32_t block_x, std::int32_t block_y, std::int32_t level) const noexcept;

    [[nodiscard]] std::int32_t block_columns() const noexcept {
        return static_cast<std::int32_t>(spans_.size());
    }

private:
    TerrainMosaic() = default;

    struct Block {
        std::array<std::uint8_t, 38> sub{};
        std::array<std::uint8_t, 3> road{};
        std::uint8_t footprint = 0;
        std::uint32_t rank     = 0;
    };

    [[nodiscard]] std::int32_t block_index(std::int32_t block_x, std::int32_t block_y, std::int32_t level) const noexcept;

    std::vector<formats::GridSpan> spans_;
    std::vector<std::int32_t> base_;
    std::int32_t stride_ = 0;
    std::int32_t levels_ = 0;
    std::vector<Block> blocks_;

    std::vector<TerrainPaint> paints_;
    std::vector<std::uint32_t> cell_;
    std::vector<formats::GridSpan> cell_spans_;
    std::vector<std::int32_t> cell_base_;
    std::int32_t cell_stride_ = 0;
    std::int32_t width_       = 0;
};

}
