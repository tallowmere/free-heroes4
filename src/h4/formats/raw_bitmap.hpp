#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::formats {

class RawBitmap {
public:
    [[nodiscard]] static core::Result<RawBitmap> parse(std::span<const std::byte> data);

    [[nodiscard]] core::Size size() const noexcept {
        return size_;
    }
    [[nodiscard]] std::span<const std::uint8_t> rgb() const noexcept {
        return rgb_;
    }

private:
    RawBitmap(core::Size size, std::vector<std::uint8_t> rgb) : size_{size}, rgb_{std::move(rgb)} {
    }

    core::Size size_;
    std::vector<std::uint8_t> rgb_;
};

}
