#pragma once

#include <cstdint>

#include "h4/graphics/color.hpp"

namespace h4::graphics {

class Color565 {
public:
    constexpr Color565() noexcept = default;
    constexpr explicit Color565(std::uint16_t bits) noexcept : bits_{bits} {
    }

    [[nodiscard]] static constexpr Color565 from_rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
        return Color565{static_cast<std::uint16_t>((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3))};
    }
    [[nodiscard]] static constexpr Color565 from(Rgba c) noexcept {
        return from_rgb(c.r, c.g, c.b);
    }

    [[nodiscard]] constexpr std::uint16_t bits() const noexcept {
        return bits_;
    }
    [[nodiscard]] constexpr std::uint8_t red() const noexcept {
        return static_cast<std::uint8_t>(bits_ >> 11 & 0x1F);
    }
    [[nodiscard]] constexpr std::uint8_t green() const noexcept {
        return static_cast<std::uint8_t>(bits_ >> 5 & 0x3F);
    }
    [[nodiscard]] constexpr std::uint8_t blue() const noexcept {
        return static_cast<std::uint8_t>(bits_ & 0x1F);
    }

    [[nodiscard]] constexpr Rgba to_rgba() const noexcept {
        return Rgba{static_cast<std::uint8_t>(red() * 255 / 31),
                    static_cast<std::uint8_t>(green() * 255 / 63),
                    static_cast<std::uint8_t>(blue() * 255 / 31),
                    255};
    }

    friend constexpr bool operator==(Color565, Color565) noexcept = default;

private:
    std::uint16_t bits_ = 0;
};

[[nodiscard]] constexpr std::uint32_t coverage_weight(std::uint8_t nibble) noexcept {
    return static_cast<std::uint32_t>(nibble + (nibble >> 3));
}

[[nodiscard]] constexpr Color565 lerp(Color565 dst, Color565 src, std::uint32_t weight) noexcept {
    if (weight >= 16)
        return src;
    const auto mix = [weight](std::uint32_t d, std::uint32_t s) { return (s * weight + d * (16 - weight)) >> 4; };
    return Color565{static_cast<std::uint16_t>(mix(dst.red(), src.red()) << 11 | mix(dst.green(), src.green()) << 5 |
                                               mix(dst.blue(), src.blue()))};
}

}
