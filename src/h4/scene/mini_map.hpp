#pragma once

#include <cstdint>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/graphics/surface565.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::graphics {
class Canvas;
}
namespace h4::model {
class Map;
class Shroud;
}

namespace h4::scene {

class MapRenderer;

class MiniMap {
public:
    MiniMap(const assets::Assets& assets, const model::Map& map, const MapRenderer& renderer, core::Size panel, std::int32_t level = 0);

    void paint(const graphics::Canvas& canvas, core::Point at, const core::Rect& view, core::Size extent) const;

    [[nodiscard]] const graphics::Surface565& image() const noexcept {
        return image_;
    }

    [[nodiscard]] static core::Point to_panel(core::Point box_point, core::Size extent, core::Size panel) noexcept {
        return {box_point.x * panel.width / extent.width, box_point.y * panel.height / extent.height};
    }
    [[nodiscard]] static core::Point to_box(core::Point panel_point, core::Size panel, core::Size extent) noexcept {
        return {extent.width * panel_point.x / panel.width, extent.height * panel_point.y / panel.height};
    }

    [[nodiscard]] static core::Rect tile_rect(std::int32_t col, std::int32_t row, std::int32_t s, core::Size panel) noexcept;

private:
    graphics::Surface565 image_;
};

}
