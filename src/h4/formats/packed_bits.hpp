#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace h4::formats {

[[nodiscard]] constexpr bool packed_bit(std::span<const std::byte> bytes, std::size_t index) noexcept {
    return index / 8 < bytes.size() && ((static_cast<std::uint8_t>(bytes[index / 8]) >> (index & 7U)) & 1U) != 0;
}

}
