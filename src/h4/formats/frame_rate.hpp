#pragma once

#include <cstdint>

namespace h4::formats {

[[nodiscard]] constexpr std::uint32_t frame_ms_of(std::uint32_t per_second) noexcept {
    return per_second == 0 ? 0U : 1000U / per_second;
}

}
