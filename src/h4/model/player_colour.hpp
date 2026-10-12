#pragma once

#include <cstddef>
#include <cstdint>

namespace h4::model {

enum class PlayerColour : std::uint8_t { red, blue, green, orange, purple, teal };
inline constexpr std::size_t kPlayerColours = 6;

}
