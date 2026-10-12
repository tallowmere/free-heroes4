#include "h4/graphics/text.hpp"

#include <algorithm>

#include "h4/formats/font.hpp"

namespace h4::graphics {

std::int32_t measure_text(const formats::Font& font, std::string_view text) noexcept {
    std::int32_t extent      = 0;
    std::int32_t pending_pad = 0;
    bool first               = true;
    for (const char c : text) {
        const formats::Glyph& g = font.glyph(c);
        if (!first)
            extent += pending_pad + g.left_bearing();
        extent += g.width();
        pending_pad = g.advance_pad();
        first       = false;
    }
    return extent;
}

std::int32_t measure_widest(const formats::Font& font, std::span<const std::string> lines) noexcept {
    std::int32_t out = 0;
    for (const std::string& line : lines)
        out = std::max(out, measure_text(font, line));
    return out;
}

core::Size wrapped_size(const formats::Font& font, std::span<const std::string> lines) noexcept {
    return {measure_widest(font, lines), static_cast<std::int32_t>(lines.size()) * font.point_size()};
}

namespace {

std::string truncate_to_width(const formats::Font& font, std::string word, std::int32_t max_width) {
    while (word.size() > 1 && measure_text(font, word) > max_width)
        word.pop_back();
    return word;
}

void wrap_paragraph(const formats::Font& font, std::string_view paragraph, std::int32_t max_width, std::vector<std::string>& lines) {
    std::string line;
    std::size_t i = 0;
    while (i < paragraph.size()) {
        std::size_t word_start = paragraph.find_first_not_of(' ', i);
        std::string_view spaces = paragraph.substr(i, (word_start == std::string_view::npos ? paragraph.size() : word_start) - i);
        if (word_start == std::string_view::npos) {
            line += spaces;
            break;
        }
        std::size_t word_end = paragraph.find(' ', word_start);
        if (word_end == std::string_view::npos)
            word_end = paragraph.size();
        const std::string_view word = paragraph.substr(word_start, word_end - word_start);

        std::string candidate = line;
        candidate += spaces;
        candidate += word;
        if (!line.empty() && measure_text(font, candidate) > max_width) {
            lines.push_back(std::move(line));
            line = truncate_to_width(font, std::string{word}, max_width);
        } else if (line.empty() && measure_text(font, candidate) > max_width) {
            line = truncate_to_width(font, std::move(candidate), max_width);
        } else {
            line = std::move(candidate);
        }
        i = word_end;
    }
    lines.push_back(std::move(line));
}

}

std::vector<std::string> wrap_text(const formats::Font& font, std::string_view text, std::int32_t max_width) {
    std::vector<std::string> lines;
    if (text.empty())
        return lines;
    std::size_t start = 0;
    for (;;) {
        const std::size_t newline = text.find('\n', start);
        wrap_paragraph(font, text.substr(start, newline - start), max_width, lines);
        if (newline == std::string_view::npos)
            break;
        start = newline + 1;
    }
    return lines;
}

std::span<const std::string> WrappedText::lines(const formats::Font& font, std::string_view text, std::int32_t max_width) {
    if (font_ != &font || width_ != max_width || text_ != text) {
        lines_ = wrap_text(font, text, max_width);
        font_  = &font;
        text_  = text;
        width_ = max_width;
    }
    return lines_;
}

}
