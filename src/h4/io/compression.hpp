#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

[[nodiscard]] core::Result<std::vector<std::byte>> inflate_exact(std::span<const std::byte> input, std::size_t expected_size);

[[nodiscard]] core::Result<std::vector<std::byte>> inflate_prefix(std::span<const std::byte> input, std::size_t limit);

[[nodiscard]] core::Result<std::vector<std::byte>> decode_lzma1_raw(std::span<const std::byte> input, std::size_t limit);

[[nodiscard]] core::Result<std::vector<std::uint8_t>> deflate_zlib(std::span<const std::uint8_t> input);

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes, std::uint32_t seed = 0) noexcept;

}
