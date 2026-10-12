#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/sub_image.hpp"

namespace h4::formats {

class AdvObject {
public:
    [[nodiscard]] static core::Result<AdvObject> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::uint16_t version() const noexcept {
        return version_;
    }
    [[nodiscard]] std::uint8_t mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] core::Size size() const noexcept {
        return {grid_w_, grid_h_};
    }
    [[nodiscard]] bool covers(std::int32_t x, std::int32_t y) const noexcept;

    [[nodiscard]] bool passage_y(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] bool passage_x(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] bool wall_y(std::int32_t x, std::int32_t y, bool bridge) const noexcept;
    [[nodiscard]] bool wall_x(std::int32_t x, std::int32_t y, bool bridge) const noexcept;
    [[nodiscard]] std::int32_t deck(std::int32_t x, std::int32_t y) const noexcept {
        if (x < 0 || y < 0 || x > grid_w_ || y > grid_h_)
            return 0;
        const auto at = static_cast<std::size_t>(x * (grid_h_ + 1) + y);
        return at < deck_.size() ? deck_[at] : 0;
    }

    [[nodiscard]] bool touches(std::int32_t x, std::int32_t y) const noexcept;

    [[nodiscard]] const BitmapGroup& group() const noexcept {
        return group_;
    }
    [[nodiscard]] core::Point anchor() const noexcept {
        return anchor_;
    }

    [[nodiscard]] std::optional<core::Point> flag_point() const noexcept {
        return flag_point_;
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

    [[nodiscard]] core::Rect frame_rect() const noexcept {
        return frame_rect_;
    }

    [[nodiscard]] std::span<const std::int16_t> span_map() const noexcept {
        return span_map_;
    }
    [[nodiscard]] std::int32_t span_at(std::int32_t column) const noexcept;

    [[nodiscard]] std::span<const std::string, 3> classification() const noexcept {
        return std::span<const std::string, 3>{names_};
    }
    [[nodiscard]] std::span<const std::uint16_t, 3> classification_ids() const noexcept {
        return std::span<const std::uint16_t, 3>{ids_};
    }

    [[nodiscard]] bool is_bridge() const noexcept;
    [[nodiscard]] bool is_span() const noexcept {
        return is_bridge() && ids_[1] == 1;
    }

    [[nodiscard]] bool overlay() const noexcept {
        return overlay_;
    }

private:
    AdvObject() = default;

    void classify_layers();

    BitmapGroup group_;
    SubImageLayers layers_;
    core::Point anchor_{};
    std::optional<core::Point> flag_point_;
    core::Rect frame_rect_{};
    std::vector<std::int16_t> span_map_;
    std::vector<std::uint8_t> footprint_;
    std::vector<std::uint8_t> passage_y_;
    std::vector<std::uint8_t> passage_x_;
    std::vector<std::uint8_t> wall_y_;
    std::vector<std::uint8_t> wall_x_;
    std::vector<std::uint8_t> deck_;
    std::array<std::string, 3> names_;
    std::array<std::uint16_t, 3> ids_{};
    std::int32_t span_first_ = 0;
    std::uint16_t version_   = 0;
    std::int32_t grid_w_     = 0;
    std::int32_t grid_h_     = 0;
    std::uint8_t mode_       = 0;
    bool overlay_            = true;
};

}
