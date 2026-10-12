#include "h4/scene/map_render.hpp"

#include "h4/assets/assets.hpp"
#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/scene/map_edges.hpp"
#include "h4/scene/map_pick.hpp"

namespace h4::scene {

MapRenderer::MapRenderer(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen)
: map_{&map}, transitions_{assets.transitions()}, mosaic_{model::TerrainMosaic::build(map.terrain(), map.terrain_seed())},
  art_{assets}, box_{map_box(map.terrain())}, frame_{assets}, objects_{assets, map}, shroud_{assets, map, seen} {
    const formats::ScenarioGrid& grid = map.terrain();
    slopes_.assign(grid.elevations().size(), graphics::kFlatSlope);
    for (std::int32_t level = 0; level < grid.levels(); ++level)
        for (std::int32_t x = 0; x <= grid.width(); ++x) {
            const formats::GridSpan& span = grid.vertex_columns()[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; y < span.stop; ++y) {
                const std::int32_t index = grid.vertex_index(x, y, level);
                if (index < 0)
                    continue;
                const std::int32_t self  = grid.elevation(x, y, level);
                const bool first         = y == span.start;
                const bool last          = y + 1 >= span.stop;
                const std::int32_t above = first ? self : grid.elevation(x, y - 1, level);
                const std::int32_t below = last ? self : grid.elevation(x, y + 1, level);
                std::uint16_t value      = graphics::kFlatSlope;
                if (first)
                    value = graphics::slope_coordinate(self, below);
                else if (last)
                    value = graphics::slope_coordinate(above, self);
                else
                    value = static_cast<std::uint16_t>(
                        (graphics::slope_coordinate(above, self) + graphics::slope_coordinate(self, below)) / 2
                    );
                slopes_[static_cast<std::size_t>(index)] = value;
            }
        }
}

std::uint16_t MapRenderer::slope_at(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const std::int32_t index = map_->terrain().vertex_index(x, y, level);
    if (index < 0 || static_cast<std::size_t>(index) >= slopes_.size())
        return graphics::kFlatSlope;
    return slopes_[static_cast<std::size_t>(index)];
}

MapRenderer::Corners MapRenderer::corners_at(std::int32_t x, std::int32_t y, std::int32_t level) const {
    const formats::ScenarioGrid& grid = map_->terrain();
    return {graphics::CornerLifts{grid.elevation(x + 1, y, level),
                                  grid.elevation(x, y, level),
                                  grid.elevation(x, y + 1, level),
                                  grid.elevation(x + 1, y + 1, level)},
            graphics::CornerSlopes{
                slope_at(x + 1, y, level), slope_at(x, y, level), slope_at(x, y + 1, level), slope_at(x + 1, y + 1, level)
            }};
}

core::Point MapRenderer::anchor(std::int32_t x, std::int32_t y) const noexcept {
    return box_anchor(x, y, map_->dimension());
}

std::optional<core::Point> MapRenderer::tile_at(core::Point point, std::int32_t level) const {
    const std::optional<core::Point> tile = tile_under(map_->terrain(), point, level);
    if (!tile.has_value() || map_->terrain().tile(tile->x, tile->y, level) == nullptr)
        return std::nullopt;
    return tile;
}

std::optional<std::uint32_t> MapRenderer::sprite_at(core::Point point, std::int32_t level, std::uint32_t now) const {
    const model::Shroud* seen = shroud_.plane();
    if (seen != nullptr)
        if (const std::optional<core::Point> tile = tile_at(point, level);
            tile.has_value() && seen->level(tile->x, tile->y, level) == model::kFogHidden)
            return std::nullopt;
    return objects_.pick(point, level, seen, now);
}

core::Point MapRenderer::centre_of(const model::MapActor& actor) const noexcept {
    if (actor.kind == model::ActorKind::town)
        if (const std::optional<core::Point> pin = objects_.pin_of(actor); pin.has_value())
            return *pin;
    return pin(actor.position.x, actor.position.y);
}

void MapRenderer::paint(const graphics::Canvas& canvas, core::Point camera, std::int32_t level, std::uint32_t now) const {
    const core::Rect box = core::Rect::from_edges(
        -camera.x, -kRaiseAllowance - camera.y, box_.width() - camera.x, box_.height() - camera.y
    );
    const core::Rect scene = frame_.paint(canvas, box);
    if (scene.empty())
        return;
    const graphics::Canvas inside = canvas.clipped_to(scene);

    const formats::ScenarioGrid& grid = map_->terrain();
    const std::int32_t width          = grid.width();

    for (std::int32_t x = 0; x < width; ++x) {
        const formats::GridSpan& span = grid.columns()[static_cast<std::size_t>(x)];
        for (std::int32_t y = span.start; y < span.stop; ++y) {
            const formats::GridTile* tile = grid.tile(x, y, level);
            if (tile == nullptr)
                continue;

            const core::Point anchor = this->anchor(x, y) - camera;
            if (!tile_reaches(anchor, scene))
                continue;

            const Corners corners = corners_at(x, y, level);

            if (tile->misc != 0 && !tile->flag) {
                graphics::fill_terrain_tile(inside, anchor, corners.lifts, graphics::Color565{});
                continue;
            }

            if (tile->road == 0 || tile->overlay_b_count != 0) {
                paint_ground(inside, *tile, x, y, level, anchor, corners);
                paint_overlay(inside, *tile, x, y, level, anchor, corners);
            }
            paint_road(inside, *tile, x, y, level, anchor, corners);
        }
    }

    paint_map_edges(inside, shroud_.texture(), grid, level, camera);
    objects_.paint(inside, camera, level, shroud_.plane(), now);
    shroud_.paint(inside, camera, level);
}

core::Point MapRenderer::army_pin(core::Point cell, std::int32_t level, bool bridge) const {
    return box_pin(cell, map_->dimension(), objects_.army_height(cell, level, bridge));
}

ArmyFigure MapRenderer::army_figure(std::uint32_t actor) const {
    return ArmyFigure{
        objects_.army_walk(actor),
        [this, actor](ArmyGroup group, int facing) { return objects_.army_frames(actor, group, facing); },
        [this, actor](ArmyGroup group) { return objects_.army_frame_ms(actor, group); },
        [this](core::Point cell, std::int32_t level, bool bridge) { return army_pin(cell, level, bridge); },
    };
}

}
