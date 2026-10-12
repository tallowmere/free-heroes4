#include "h4/graphics/rgba_image.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/color565.hpp"
#include "h4/graphics/nearest.hpp"

namespace h4::graphics {

RgbaImage rasterize_layer(const formats::Layer& layer, const core::Rect& box) {
    RgbaImage image{box.empty() ? core::Size{} : box.size(), {}};
    image.pixels.assign(static_cast<std::size_t>(image.size.width) * static_cast<std::size_t>(image.size.height) * 4, 0);
    if (image.size.empty() || layer.is_placeholder())
        return image;

    const auto palette       = layer.palette();
    const auto pixels        = layer.pixels();
    const auto rows          = layer.rows();
    const core::Point origin = layer.bounds().origin() - box.origin();
    for (std::size_t y = 0; y < rows.size(); ++y) {
        const formats::RowSpan& run = rows[y];
        const std::int32_t py       = origin.y + static_cast<std::int32_t>(y);
        if (py < 0 || py >= image.size.height)
            continue;
        for (std::size_t i = 0; i < run.length(); ++i) {
            const std::size_t index  = run.offset + i;
            const std::uint8_t cover = layer.coverage(index);
            const std::int32_t px    = origin.x + run.x0 + static_cast<std::int32_t>(i);
            if (cover == 0 || px < 0 || px >= image.size.width)
                continue;
            const formats::PaletteColor c = palette[pixels[index]];
            const Rgba rgb                = Color565::from_rgb(c.r, c.g, c.b).to_rgba();
            const auto at                 = (static_cast<std::size_t>(py) * static_cast<std::size_t>(image.size.width) +
                                             static_cast<std::size_t>(px)) *
                4;
            image.pixels[at]     = rgb.r;
            image.pixels[at + 1] = rgb.g;
            image.pixels[at + 2] = rgb.b;
            image.pixels[at + 3] = static_cast<std::uint8_t>(std::min<std::uint32_t>(255, coverage_weight(cover) * 255 / 16));
        }
    }
    return image;
}

RgbaImage scale_nearest(const RgbaImage& image, double scale) {
    const auto side = [scale](std::int32_t length) {
        return std::max(1, static_cast<std::int32_t>(std::lround(length * scale)));
    };
    RgbaImage out{{side(image.size.width), side(image.size.height)}, {}};
    out.pixels.resize(static_cast<std::size_t>(out.size.width) * static_cast<std::size_t>(out.size.height) * 4);
    if (image.size.empty())
        return out;
    const auto source = [scale](std::int32_t at, std::int32_t length) {
        return static_cast<std::size_t>(std::min(length - 1, nearest_source(at, scale)));
    };
    const auto width = static_cast<std::size_t>(image.size.width);
    for (std::int32_t y = 0; y < out.size.height; ++y) {
        const std::size_t row = source(y, image.size.height) * width;
        for (std::int32_t x = 0; x < out.size.width; ++x) {
            const std::size_t from = (row + source(x, image.size.width)) * 4;
            const std::size_t to   = (static_cast<std::size_t>(y) * static_cast<std::size_t>(out.size.width) +
                                      static_cast<std::size_t>(x)) *
                4;
            std::copy_n(image.pixels.begin() + static_cast<std::ptrdiff_t>(from),
                        4,
                        out.pixels.begin() + static_cast<std::ptrdiff_t>(to));
        }
    }
    return out;
}

}
