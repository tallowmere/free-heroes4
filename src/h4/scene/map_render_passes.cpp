#include <algorithm>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/scene/map_render.hpp"

namespace h4::scene {

namespace {

[[nodiscard]] bool one_block(std::uint8_t terrain) noexcept {
    return terrain >= 12 && terrain <= 14;
}

struct Piece {
    std::uint8_t slot   = 0;
    std::uint16_t entry = 0;
};

constexpr std::uint8_t kWaterEdge = 4;

[[nodiscard]] Piece piece_of(const formats::TileOverlay& overlay) noexcept {
    return {overlay.tail, overlay.field};
}

}

std::uint8_t MapRenderer::self_slot(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const std::int32_t block_x = x / model::TerrainMosaic::kBlock;
    const std::int32_t block_y = y / model::TerrainMosaic::kBlock;
    const std::int32_t reach   = mosaic_.footprint(block_x, block_y, level);
    const std::int32_t column  = x - block_x * model::TerrainMosaic::kBlock + 4 - reach;
    const std::int32_t row     = y - block_y * model::TerrainMosaic::kBlock + 4 - reach;
    return static_cast<std::uint8_t>(column + row * 10);
}

void MapRenderer::paint_ground(const graphics::Canvas& canvas,
                               const formats::GridTile& tile,
                               std::int32_t x,
                               std::int32_t y,
                               std::int32_t level,
                               core::Point anchor,
                               const Corners& corners) const {
    if (one_block(tile.terrain)) {
        if (const formats::TerrainTexture* art = art_.ground(tile.terrain, tile.variant, 0); art != nullptr)
            graphics::blit_terrain_tile(
                canvas, *art, static_cast<std::size_t>((y % 4 + 3) * 10 + (x % 4 + 3)), anchor, corners.lifts, corners.slopes
            );
        return;
    }

    for (const model::TerrainPaint& paint : mosaic_.paints(x, y, level)) {
        const std::uint8_t sub = mosaic_.sub_variant(paint.block_x, paint.block_y, level, tile.terrain, tile.variant);
        const formats::TerrainTexture* art = art_.ground(tile.terrain, tile.variant, sub);
        if (art == nullptr)
            continue;
        graphics::blit_terrain_tile(canvas, *art, paint.slot, anchor, corners.lifts, corners.slopes);
    }
}

void MapRenderer::paint_overlay(const graphics::Canvas& canvas,
                                const formats::GridTile& tile,
                                std::int32_t x,
                                std::int32_t y,
                                std::int32_t level,
                                core::Point anchor,
                                const Corners& corners) const {
    if (transitions_ == nullptr)
        return;
    const std::span<const formats::TileOverlay> all = map_->terrain().overlays_a();
    const core::Rect clip = canvas.local_clip();
    const std::pair<std::size_t, std::size_t> visible = {static_cast<std::size_t>(std::clamp(clip.left() - anchor.x, 0, 64)),
                                                         static_cast<std::size_t>(std::clamp(clip.right() - anchor.x, 0, 64))};

    for (std::uint8_t i = 0; i < tile.overlay_a_count; ++i) {
        const formats::TileOverlay& overlay = all[tile.overlay_a + i];
        const Piece piece                   = piece_of(overlay);
        const formats::TransitionSet* set   = transitions_->slot(piece.slot);
        if (set == nullptr)
            continue;
        const formats::TransitionEntry* mask = set->entry(piece.entry);
        if (mask == nullptr)
            continue;

        if (one_block(overlay.terrain)) {
            if (const formats::TerrainTexture* art = art_.ground(overlay.terrain, overlay.variant, 0); art != nullptr)
                graphics::blit_transition_tile(canvas,
                                               *art,
                                               static_cast<std::size_t>((y % 4 + 3) * 10 + (x % 4 + 3)),
                                               anchor,
                                               corners.lifts,
                                               corners.slopes,
                                               {mask});
            continue;
        }
        const auto paints = mosaic_.paints(x, y, level);
        const bool merged = mask->kind() == kWaterEdge && paints.size() >= 2;
        for (const model::TerrainPaint& paint : paints) {
            const std::uint8_t sub = mosaic_.sub_variant(paint.block_x, paint.block_y, level, overlay.terrain, overlay.variant);
            const formats::TerrainTexture* art = art_.ground(overlay.terrain, overlay.variant, sub);
            if (art == nullptr)
                continue;
            if (merged)
                offscreen_.render(*art, paint.slot, visible.first, visible.second);
            else
                graphics::blit_transition_tile(canvas, *art, paint.slot, anchor, corners.lifts, corners.slopes, {mask});
        }
        if (merged)
            graphics::composite_transition_tile(canvas, offscreen_, anchor, corners.lifts, corners.slopes, *mask);
    }
}

void MapRenderer::paint_road(const graphics::Canvas& canvas,
                             const formats::GridTile& tile,
                             std::int32_t x,
                             std::int32_t y,
                             std::int32_t level,
                             core::Point anchor,
                             const Corners& corners) const {
    if (transitions_ == nullptr)
        return;
    const std::span<const formats::TileOverlay> all = map_->terrain().overlays_b();

    graphics::TileStencil stencil;
    bool holes = false;
    for (std::uint8_t i = 0; i < tile.overlay_b_count; ++i) {
        const formats::TileOverlay& overlay = all[tile.overlay_b + i];
        if (overlay.terrain != 0)
            continue;
        const Piece piece = piece_of(overlay);
        if (const formats::TransitionSet* set = transitions_->slot(piece.slot); set != nullptr)
            if (const formats::TransitionEntry* mask = set->entry(piece.entry); mask != nullptr) {
                stencil.clear(*mask);
                holes = true;
            }
    }

    if (tile.road != 0) {
        const std::uint8_t sub = mosaic_.road_sub_variant(
            x / model::TerrainMosaic::kBlock, y / model::TerrainMosaic::kBlock, level, tile.road
        );
        if (const formats::TerrainTexture* art = art_.road(tile.road, sub); art != nullptr) {
            if (!holes)
                graphics::blit_terrain_tile(canvas, *art, self_slot(x, y, level), anchor, corners.lifts, corners.slopes);
            else
                graphics::blit_transition_tile(
                    canvas, *art, self_slot(x, y, level), anchor, corners.lifts, corners.slopes, {nullptr, &stencil}
                );
        }
    }

    for (std::uint8_t i = 0; i < tile.overlay_b_count; ++i) {
        const formats::TileOverlay& overlay = all[tile.overlay_b + i];
        if (overlay.terrain == 0)
            continue;
        const Piece piece                 = piece_of(overlay);
        const formats::TransitionSet* set = transitions_->slot(piece.slot);
        if (set == nullptr)
            continue;
        const formats::TransitionEntry* mask = set->entry(piece.entry);
        if (mask == nullptr)
            continue;

        const std::uint8_t sub = mosaic_.road_sub_variant(
            x / model::TerrainMosaic::kBlock, y / model::TerrainMosaic::kBlock, level, overlay.terrain
        );
        if (const formats::TerrainTexture* art = art_.road(overlay.terrain, sub); art != nullptr)
            graphics::blit_transition_tile(
                canvas, *art, self_slot(x, y, level), anchor, corners.lifts, corners.slopes, {mask, tile.road != 0 && holes ? &stencil : nullptr}
            );
    }
}

}
