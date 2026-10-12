#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/graphics/terrain_blit.hpp"
#include "h4/model/map.hpp"
#include "h4/model/terrain_mosaic.hpp"
#include "h4/scene/map_frame.hpp"
#include "h4/scene/map_hover.hpp"
#include "h4/scene/map_lattice.hpp"
#include "h4/scene/map_objects.hpp"
#include "h4/scene/map_shroud.hpp"
#include "h4/scene/map_terrain_art.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class ScenarioGrid;
class TerrainTexture;
class TransitionLibrary;
class TransitionSet;
}
namespace h4::graphics {
class Canvas;
}

namespace h4::scene {

inline constexpr std::int32_t kCameraMargin = 50;

class MapRenderer {
public:
    MapRenderer(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen = nullptr);

    void paint(const graphics::Canvas& canvas, core::Point camera, std::int32_t level, std::uint32_t now = 0) const;

    void select_army(const model::MapActor* army) {
        objects_.select(army);
    }
    void show_path(std::span<const PathMark> marks) {
        objects_.show_path(marks);
    }

    void pose_army(std::uint32_t actor, core::Point cell, std::int32_t level, const ArmyPose& pose) {
        objects_.pose_army(actor, cell, level, pose);
    }
    void remove_object(std::uint32_t actor) noexcept {
        objects_.remove(actor);
    }
    [[nodiscard]] const MapObjects& objects() const noexcept {
        return objects_;
    }
    [[nodiscard]] core::Point army_pin(core::Point cell, std::int32_t level, bool bridge = false) const;
    [[nodiscard]] ArmyFigure army_figure(std::uint32_t actor) const;

    [[nodiscard]] core::Size extent() const noexcept {
        return box_.size();
    }

    [[nodiscard]] core::Point anchor(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] core::Point pin(std::int32_t x, std::int32_t y) const noexcept {
        return anchor(x, y) + core::Point{kTileWidth / 2, 0};
    }
    [[nodiscard]] core::Point centre_of(const model::MapActor& actor) const noexcept;

    [[nodiscard]] std::optional<core::Point> tile_at(core::Point point, std::int32_t level) const;
    [[nodiscard]] std::optional<std::uint32_t> sprite_at(core::Point point, std::int32_t level, std::uint32_t now) const;
    [[nodiscard]] MapHover hover_at(core::Point point, std::int32_t level, std::uint32_t now) const {
        return {point, level, tile_at(point, level), sprite_at(point, level, now)};
    }

    [[nodiscard]] std::uint16_t slope_at(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;
    [[nodiscard]] const model::Shroud* shroud_plane() const noexcept {
        return shroud_.plane();
    }

private:
    struct Corners {
        graphics::CornerLifts lifts;
        graphics::CornerSlopes slopes;
    };

    [[nodiscard]] Corners corners_at(std::int32_t x, std::int32_t y, std::int32_t level) const;

    void paint_ground(const graphics::Canvas& canvas,
                      const formats::GridTile& tile,
                      std::int32_t x,
                      std::int32_t y,
                      std::int32_t level,
                      core::Point anchor,
                      const Corners& corners) const;
    void paint_overlay(const graphics::Canvas& canvas,
                       const formats::GridTile& tile,
                       std::int32_t x,
                       std::int32_t y,
                       std::int32_t level,
                       core::Point anchor,
                       const Corners& corners) const;
    void paint_road(const graphics::Canvas& canvas,
                    const formats::GridTile& tile,
                    std::int32_t x,
                    std::int32_t y,
                    std::int32_t level,
                    core::Point anchor,
                    const Corners& corners) const;

    [[nodiscard]] std::uint8_t self_slot(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;

    const model::Map* map_;
    const formats::TransitionLibrary* transitions_ = nullptr;
    model::TerrainMosaic mosaic_;
    TerrainArt art_;
    core::Rect box_;
    MapFrame frame_;
    MapObjects objects_;
    MapShroud shroud_;
    std::vector<std::uint16_t> slopes_;
    mutable graphics::TileOffscreen offscreen_;
};

}
