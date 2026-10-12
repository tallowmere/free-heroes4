#pragma once

#include <cstdint>
#include <string_view>

#include "h4/core/geometry.hpp"
#include "h4/graphics/coverage.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/model/shroud.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class TransitionLibrary;
}
namespace h4::graphics {
class Canvas;
}
namespace h4::model {
class Map;
}

namespace h4::scene {

inline constexpr std::string_view kShroudGroup = "shroud";

class MapShroud {
public:
    MapShroud(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen);

    [[nodiscard]] bool active() const noexcept;

    [[nodiscard]] const model::Shroud* plane() const noexcept {
        return seen_;
    }

    [[nodiscard]] const graphics::Surface565& texture() const noexcept {
        return fog_;
    }

    void paint(const graphics::Canvas& canvas, core::Point camera, std::int32_t level) const;

private:
    void draw_tile(graphics::CoverageSurface& surface,
                   std::int32_t x,
                   std::int32_t y,
                   std::int32_t level,
                   core::Point anchor,
                   const graphics::CornerLifts& lifts) const;
    void paint_margins(core::Point origin, std::int32_t level) const;

    const model::Map* map_;
    const formats::TransitionLibrary* transitions_ = nullptr;
    const model::Shroud* seen_;
    graphics::Surface565 fog_;
    mutable graphics::CoverageSurface plane_;
};

}
