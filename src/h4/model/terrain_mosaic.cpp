#include "h4/model/terrain_mosaic.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <numeric>
#include <string_view>

#include "h4/model/roller.hpp"

namespace h4::model {

namespace {

constexpr std::size_t kClasses  = 19;
constexpr std::size_t kVariants = 2;

constexpr std::array<std::uint8_t, kClasses> kVariantsOf{2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
constexpr std::array<std::uint8_t, kClasses> kPayloadsOf{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2};

constexpr std::array<std::string_view, kClasses> kFamilies{"water",
                                                           "grass",
                                                           "rough",
                                                           "swamp",
                                                           "lava",
                                                           "snow",
                                                           "sand",
                                                           "dirt",
                                                           "subterranean",
                                                           "river.water",
                                                           "river.lava",
                                                           "river.ice",
                                                           "magic.all",
                                                           "magic.life",
                                                           "magic.order",
                                                           "magic.death",
                                                           "magic.chaos",
                                                           "magic.nature",
                                                           "special.personal_"
                                                           "combat"};

constexpr std::array<std::string_view, 3> kRoadFamilies{"road.gravel", "road.dirt", "road.cobblestone"};

struct Reach {
    bool covers = false;
    bool window = false;
};

[[nodiscard]] constexpr Reach reach_from_low(std::int32_t d, std::int32_t f) noexcept {
    return {d < f + 2, d < f};
}
[[nodiscard]] constexpr Reach reach_from_high(std::int32_t d, std::int32_t f) noexcept {
    return {f <= d, f + 2 <= d};
}

struct Candidate {
    std::int32_t block_x = 0;
    std::int32_t block_y = 0;
    std::uint32_t rank   = 0;
    bool window          = false;
};

}

std::uint8_t terrain_payload_count(std::uint8_t terrain, std::uint8_t variant) noexcept {
    if (terrain >= kClasses || variant >= kVariantsOf[terrain])
        return 0;
    return kPayloadsOf[terrain];
}

std::string terrain_payload_name(std::uint8_t terrain, std::uint8_t variant, std::uint8_t sub) {
    if (terrain >= kClasses)
        return {};
    const std::string_view family = kFamilies[terrain];
    if (terrain < 9)
        return std::format("{}.{}.{}", family, variant + 1, sub + 1);
    if (kPayloadsOf[terrain] == 1)
        return std::string{family};
    return std::format("{}.{}", family, sub + 1);
}

std::string road_payload_name(std::uint8_t road, std::uint8_t sub) {
    if (road < 1 || road > kRoadFamilies.size())
        return {};
    return std::format("{}.{}", kRoadFamilies[road - 1], sub + 1);
}

std::int32_t TerrainMosaic::block_index(std::int32_t block_x, std::int32_t block_y, std::int32_t level) const noexcept {
    if (block_x < 0 || static_cast<std::size_t>(block_x) >= base_.size())
        return -1;
    return base_[static_cast<std::size_t>(block_x)] + stride_ * level + block_y;
}

std::uint8_t
TerrainMosaic::sub_variant(std::int32_t block_x, std::int32_t block_y, std::int32_t level, std::uint8_t terrain, std::uint8_t variant) const noexcept {
    if (terrain >= kClasses || variant >= kVariants)
        return 0;
    const std::int32_t index = block_index(block_x, block_y, level);
    if (index < 0 || static_cast<std::size_t>(index) >= blocks_.size())
        return 0;
    return blocks_[static_cast<std::size_t>(index)].sub[terrain * kVariants + variant];
}

std::uint8_t TerrainMosaic::road_sub_variant(std::int32_t block_x, std::int32_t block_y, std::int32_t level, std::uint8_t road) const noexcept {
    if (road < 1 || road > 3)
        return 0;
    const std::int32_t index = block_index(block_x, block_y, level);
    if (index < 0 || static_cast<std::size_t>(index) >= blocks_.size())
        return 0;
    return blocks_[static_cast<std::size_t>(index)].road[road - 1];
}

std::uint8_t TerrainMosaic::footprint(std::int32_t block_x, std::int32_t block_y, std::int32_t level) const noexcept {
    const std::int32_t index = block_index(block_x, block_y, level);
    if (index < 0 || static_cast<std::size_t>(index) >= blocks_.size())
        return 0;
    return blocks_[static_cast<std::size_t>(index)].footprint;
}

std::span<const TerrainPaint> TerrainMosaic::paints(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || level < 0 || level >= levels_)
        return {};
    const formats::GridSpan& span = cell_spans_[static_cast<std::size_t>(x)];
    if (y < span.start || y >= span.stop)
        return {};
    const std::size_t cell = static_cast<std::size_t>(cell_base_[static_cast<std::size_t>(x)] + cell_stride_ * level + y);
    if (cell + 1 >= cell_.size())
        return {};
    return std::span{paints_}.subspan(cell_[cell], cell_[cell + 1] - cell_[cell]);
}

TerrainMosaic TerrainMosaic::build(const formats::ScenarioGrid& grid, std::uint32_t seed) {
    TerrainMosaic out;
    out.levels_ = grid.levels();
    out.width_  = grid.width();
    if (out.width_ == 0 || out.levels_ == 0)
        return out;

    const std::span<const formats::GridSpan> cells = grid.columns();
    out.cell_spans_.assign(cells.begin(), cells.end());
    out.cell_base_.reserve(cells.size());
    for (const formats::GridSpan& span : cells) {
        out.cell_base_.push_back(out.cell_stride_ - span.start);
        out.cell_stride_ += span.size();
    }

    const std::int32_t columns = (out.width_ + kBlock - 1) / kBlock;
    out.spans_.reserve(static_cast<std::size_t>(columns));
    out.base_.reserve(static_cast<std::size_t>(columns));
    for (std::int32_t i = 0; i < columns; ++i) {
        formats::GridSpan span{columns, 0};
        for (std::int32_t x = i * kBlock; x < i * kBlock + kBlock && x < out.width_; ++x) {
            const formats::GridSpan& cell = cells[static_cast<std::size_t>(x)];
            span.start                    = std::min(span.start, cell.start / kBlock);
            span.stop                     = std::max(span.stop, (cell.stop + kBlock - 1) / kBlock);
        }
        out.base_.push_back(out.stride_ - span.start);
        out.stride_ += span.size();
        out.spans_.push_back(span);
    }

    Roller roller{seed};
    const std::size_t count = static_cast<std::size_t>(out.stride_) * static_cast<std::size_t>(out.levels_);
    std::vector<std::uint32_t> ranks(count);
    std::iota(ranks.begin(), ranks.end(), 0U);
    for (std::size_t i = 1; i < ranks.size(); ++i)
        std::swap(ranks[i], ranks[roller.next_index(i)]);

    out.blocks_.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        Block& block = out.blocks_[i];
        block.rank   = ranks[i];
        for (std::size_t terrain = 0; terrain < kClasses; ++terrain)
            for (std::uint8_t variant = 0; variant < kVariantsOf[terrain]; ++variant)
                block.sub[terrain * kVariants + variant] = static_cast<std::uint8_t>(roller.next_index(kPayloadsOf[terrain]));
        for (std::uint8_t road = 0; road < 3; ++road)
            block.road[road] = static_cast<std::uint8_t>(roller.next(2));
        block.footprint = static_cast<std::uint8_t>(roller.next_twice(3));
    }

    out.cell_.reserve(static_cast<std::size_t>(out.cell_stride_) * static_cast<std::size_t>(out.levels_) + 1);
    std::vector<Candidate> candidates;
    for (std::int32_t level = 0; level < out.levels_; ++level) {
        for (std::int32_t x = 0; x < out.width_; ++x) {
            const formats::GridSpan& span = cells[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; y < span.stop; ++y) {
                out.cell_.push_back(static_cast<std::uint32_t>(out.paints_.size()));
                const std::int32_t bx   = x / kBlock;
                const std::int32_t by   = y / kBlock;
                const std::int32_t dx   = x - bx * kBlock;
                const std::int32_t dy   = y - by * kBlock;
                const std::uint32_t own = out.blocks_[static_cast<std::size_t>(out.block_index(bx, by, level))].rank;

                candidates.clear();
                candidates.push_back({bx, by, own, true});
                for (std::int32_t sx = -1; sx <= 1; ++sx)
                    for (std::int32_t sy = -1; sy <= 1; ++sy) {
                        if (sx == 0 && sy == 0)
                            continue;
                        const std::int32_t nx = bx + sx;
                        const std::int32_t ny = by + sy;
                        if (nx < 0 || nx >= columns)
                            continue;
                        const formats::GridSpan& nb = out.spans_[static_cast<std::size_t>(nx)];
                        if (ny < nb.start || ny >= nb.stop)
                            continue;
                        const Block& block = out.blocks_[static_cast<std::size_t>(out.block_index(nx, ny, level))];
                        if (block.rank <= own)
                            continue;
                        const Reach across = sx == 0 ?
                            Reach{true, true} :
                            (sx < 0 ? reach_from_low(dx, block.footprint) : reach_from_high(dx, block.footprint));
                        const Reach down   = sy == 0 ?
                            Reach{true, true} :
                            (sy < 0 ? reach_from_low(dy, block.footprint) : reach_from_high(dy, block.footprint));
                        if (!across.covers || !down.covers)
                            continue;
                        candidates.push_back({nx, ny, block.rank, across.window && down.window});
                    }

                std::ranges::sort(candidates, std::ranges::greater{}, &Candidate::rank);
                std::size_t owner = 0;
                while (owner + 1 < candidates.size() && !candidates[owner].window)
                    ++owner;
                for (std::size_t i = owner + 1; i-- > 0;) {
                    const Candidate& candidate = candidates[i];
                    const std::int32_t
                        reach = out.blocks_[static_cast<std::size_t>(out.block_index(candidate.block_x, candidate.block_y, level))]
                                    .footprint;
                    const std::int32_t sx = x - candidate.block_x * kBlock + kBlock - reach;
                    const std::int32_t sy = y - candidate.block_y * kBlock + kBlock - reach;
                    out.paints_.push_back({candidate.block_x, candidate.block_y, static_cast<std::uint8_t>(sy * 10 + sx)});
                }
            }
        }
    }
    out.cell_.push_back(static_cast<std::uint32_t>(out.paints_.size()));
    return out;
}

}
