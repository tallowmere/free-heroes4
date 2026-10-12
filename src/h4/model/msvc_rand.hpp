#pragma once

#include <cstdint>

namespace h4::model {

[[nodiscard]] constexpr std::uint32_t msvc_rand_step(std::uint32_t state) noexcept {
    return state * 214013U + 2531011U;
}

[[nodiscard]] constexpr std::uint32_t msvc_rand_step_twice(std::uint32_t state) noexcept {
    return state * 0xA9FC6809U + 0x1E278E7AU;
}
static_assert(msvc_rand_step_twice(0) == msvc_rand_step(msvc_rand_step(0)) &&
                  msvc_rand_step_twice(1) == msvc_rand_step(msvc_rand_step(1)),
              "the fused constants are two steps of the stream");
static_assert(0U - 0x560397F7U == 0xA9FC6809U, "the multiplier the listing negates");

[[nodiscard]] constexpr std::uint32_t msvc_rand_value(std::uint32_t state) noexcept {
    return state >> 16 & 0x7FFFU;
}

}
