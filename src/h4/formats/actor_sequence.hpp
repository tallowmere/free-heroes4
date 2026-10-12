#pragma once

#include <cstddef>
#include <span>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/sub_image.hpp"

namespace h4::formats {

class ActorSequence {
public:
    [[nodiscard]] static core::Result<ActorSequence> parse(std::span<const std::byte> payload);

    [[nodiscard]] const BitmapGroup& group() const noexcept {
        return group_;
    }
    [[nodiscard]] core::Point anchor() const noexcept {
        return anchor_;
    }

    [[nodiscard]] const SubImageLayers& layers() const noexcept {
        return layers_;
    }
    [[nodiscard]] std::span<const FrameGroup> frames() const noexcept {
        return layers_.frames;
    }
    [[nodiscard]] std::span<const FrameGroup> shadows() const noexcept {
        return layers_.shadows;
    }
    [[nodiscard]] std::int32_t base_frame() const noexcept {
        return layers_.base_frame;
    }
    [[nodiscard]] std::int32_t base_shadow() const noexcept {
        return layers_.base_shadow;
    }

private:
    ActorSequence() = default;

    BitmapGroup group_;
    core::Point anchor_;
    SubImageLayers layers_;
};

}
