#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/strong.hpp"

namespace h4::platform {

using CursorImageId = core::Strong<struct CursorImageIdTag, std::uint32_t>;

class CursorImage final {
public:
    [[nodiscard]] static core::Result<CursorImage>
    make(CursorImageId id, core::Size size, core::Point hot_spot, std::vector<std::uint8_t> rgba);

    [[nodiscard]] CursorImageId id() const noexcept {
        return id_;
    }
    [[nodiscard]] core::Size size() const noexcept {
        return size_;
    }
    [[nodiscard]] core::Point hot_spot() const noexcept {
        return hot_spot_;
    }
    [[nodiscard]] std::span<const std::uint8_t> rgba() const noexcept {
        return rgba_;
    }

private:
    CursorImage(CursorImageId id, core::Size size, core::Point hot_spot, std::vector<std::uint8_t> rgba) noexcept
    : id_{id}, size_{size}, hot_spot_{hot_spot}, rgba_{std::move(rgba)} {
    }

    CursorImageId id_;
    core::Size size_;
    core::Point hot_spot_;
    std::vector<std::uint8_t> rgba_;
};

inline core::Result<CursorImage>
CursorImage::make(CursorImageId id, core::Size size, core::Point hot_spot, std::vector<std::uint8_t> rgba) {
    if (size.width <= 0 || size.height <= 0)
        return core::fail(core::ErrorCode::invalid_argument, "cursor image: empty size");
    if (rgba.size() != static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height) * 4)
        return core::fail(core::ErrorCode::invalid_argument, "cursor image: pixels do not cover it");
    if (hot_spot.x < 0 || hot_spot.y < 0 || hot_spot.x >= size.width || hot_spot.y >= size.height)
        return core::fail(core::ErrorCode::invalid_argument, "cursor image: hot spot outside it");
    return CursorImage{id, size, hot_spot, std::move(rgba)};
}

}
