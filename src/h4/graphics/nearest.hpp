#pragma once

#include <cmath>
#include <cstdint>

namespace h4::graphics {

[[nodiscard]] inline std::int32_t nearest_source(std::int32_t destination, double scale) noexcept {
    return static_cast<std::int32_t>(std::floor((destination + 0.5) / scale));
}

[[nodiscard]] inline std::int32_t first_destination(std::int32_t source, double scale) noexcept {
    return static_cast<std::int32_t>(std::ceil(source * scale - 0.5));
}

}
