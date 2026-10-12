#pragma once

#include "h4/graphics/color565.hpp"

namespace h4::graphics {

[[nodiscard]] Color565 shade(Color565 pixel, unsigned index) noexcept;

[[nodiscard]] int shade_index(int biased, int parity_a, int parity_b) noexcept;

}
