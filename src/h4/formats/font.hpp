#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {

class Glyph {
public:
    [[nodiscard]] std::int32_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::int32_t height() const noexcept {
        return height_;
    }
    [[nodiscard]] std::int32_t left_bearing() const noexcept {
        return left_bearing_;
    }
    [[nodiscard]] std::int32_t advance_pad() const noexcept {
        return advance_pad_;
    }

    [[nodiscard]] std::uint8_t body(std::int32_t x, std::int32_t y) const noexcept {
        return static_cast<std::uint8_t>(at(x, y) >> 4);
    }
    [[nodiscard]] std::uint8_t shadow(std::int32_t x, std::int32_t y) const noexcept {
        return static_cast<std::uint8_t>(at(x, y) & 0x0F);
    }

private:
    friend class Font;
    Glyph() = default;
    [[nodiscard]] std::uint8_t at(std::int32_t x, std::int32_t y) const noexcept {
        return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
    }

    std::int32_t width_        = 0;
    std::int32_t height_       = 0;
    std::int32_t left_bearing_ = 0;
    std::int32_t advance_pad_  = 0;
    std::vector<std::uint8_t> pixels_;
};

class Font {
public:
    [[nodiscard]] static core::Result<Font> parse(std::span<const std::byte> data);
    [[nodiscard]] static Font builtin();

    [[nodiscard]] std::int32_t point_size() const noexcept {
        return point_size_;
    }
    [[nodiscard]] std::uint8_t first_char() const noexcept {
        return first_char_;
    }
    [[nodiscard]] std::size_t glyph_count() const noexcept {
        return glyphs_.size();
    }

    [[nodiscard]] const Glyph& glyph(char c) const noexcept;

private:
    Font() = default;

    std::int32_t point_size_ = 0;
    std::uint8_t first_char_ = 0;
    std::vector<Glyph> glyphs_;
};

}
