#pragma once

#include <cstdint>

#include "h4/model/alignment.hpp"

namespace h4::model {

class Roller;

[[nodiscard]] Alignment drawn_alignment(std::uint8_t allowed, Roller& roller) noexcept;

}
