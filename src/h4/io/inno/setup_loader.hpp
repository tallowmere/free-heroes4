#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

[[nodiscard]] core::Result<std::size_t> setup_data_offset(std::span<const std::byte> exe);

struct SetupBlock {
    std::vector<std::byte> content;
    std::size_t end = 0;
};

[[nodiscard]] core::Result<SetupBlock> read_setup_block(std::span<const std::byte> exe, std::size_t offset);

}
