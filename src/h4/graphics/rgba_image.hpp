#pragma once

#include <cstdint>
#include <vector>

#include "h4/core/geometry.hpp"

namespace h4::formats {
class Layer;
}

namespace h4::graphics {

struct RgbaImage {
    core::Size size;
    std::vector<std::uint8_t> pixels;
};

[[nodiscard]] RgbaImage rasterize_layer(const formats::Layer& layer, const core::Rect& box);

[[nodiscard]] RgbaImage scale_nearest(const RgbaImage& image, double scale);

}
