#include "h4/screens/kit/fit_words.hpp"

#include <algorithm>

#include "h4/formats/font.hpp"
#include "h4/graphics/text.hpp"

namespace h4::screens {

std::int32_t fit_width(const formats::Font& font, std::string_view text, std::int32_t min, std::int32_t max) {
    std::int32_t longest = 0;
    std::size_t at       = 0;
    while (at < text.size()) {
        while (at < text.size() && (text[at] == ' ' || text[at] == '\n'))
            ++at;
        std::size_t end = at;
        while (end < text.size() && text[end] != ' ' && text[end] != '\n')
            ++end;
        longest = std::max(longest, graphics::measure_text(font, text.substr(at, end - at)));
        at      = end;
    }
    max                = std::max(max, 0);
    std::int32_t width = std::max(min, std::min(longest, max));
    while (true) {
        const auto lines = static_cast<std::int32_t>(graphics::wrap_text(font, text, width).size());
        if (lines <= 1 || width >= max || font.point_size() * lines * 4 <= width)
            return width;
        width = std::min(width * 3 / 2, max);
    }
}

Words lay_out(const formats::Font& font, std::string_view text, std::int32_t min, std::int32_t max) {
    Words out;
    if (text.empty())
        return out;
    out.lines = graphics::wrap_text(font, text, fit_width(font, text, min, max));
    out.size  = graphics::wrapped_size(font, out.lines);
    return out;
}

}
