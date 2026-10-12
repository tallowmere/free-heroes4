#pragma once

#include <cstdint>

namespace h4::graphics {

struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    friend constexpr bool operator==(Rgba, Rgba) noexcept = default;
};

namespace colors {
inline constexpr Rgba black{0, 0, 0, 255};
inline constexpr Rgba white{255, 255, 255, 255};
inline constexpr Rgba transparent{0, 0, 0, 0};
}

}
