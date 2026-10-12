#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

struct FrameGroup {
    std::uint32_t number = 0;
    std::vector<std::uint32_t> layers;
};

struct SubImageLayers {
    std::vector<FrameGroup> frames;
    std::vector<FrameGroup> shadows;
    std::int32_t base_frame  = -1;
    std::int32_t base_shadow = -1;
};

[[nodiscard]] SubImageLayers classify_sub_image(const BitmapGroup& group);

[[nodiscard]] core::Rect frame_extent(const BitmapGroup& group, const SubImageLayers& layers, bool shadows);

struct SubImageBody {
    BitmapGroup group;
    core::Point anchor;
};
[[nodiscard]] core::Result<SubImageBody> read_sub_image_body(std::span<const std::byte> payload, core::ByteReader& in, bool v1_name);

}
