#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"

namespace h4::formats {
class Font;
}

namespace h4::screens {

[[nodiscard]] std::int32_t fit_width(const formats::Font& font, std::string_view text, std::int32_t min, std::int32_t max);

struct Words {
    std::vector<std::string> lines;
    core::Size size;
};

[[nodiscard]] Words lay_out(const formats::Font& font, std::string_view text, std::int32_t min, std::int32_t max);

}
