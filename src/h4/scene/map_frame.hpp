#pragma once

#include <string_view>

#include "h4/core/geometry.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
}
namespace h4::graphics {
class Canvas;
}

namespace h4::scene {

inline constexpr std::string_view kFrameGroup = "map_frame";

class MapFrame {
public:
    explicit MapFrame(const assets::Assets& assets);

    [[nodiscard]] core::Rect paint(const graphics::Canvas& canvas, const core::Rect& box) const;

private:
    const formats::Layer* top_    = nullptr;
    const formats::Layer* bottom_ = nullptr;
    const formats::Layer* left_   = nullptr;
    const formats::Layer* right_  = nullptr;
    const formats::Layer* corner_ = nullptr;
};

}
