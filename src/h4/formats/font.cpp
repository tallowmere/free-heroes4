#include "h4/formats/font.hpp"

#include <format>

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

namespace {
constexpr std::uint16_t kSupportedVersion = 1;
constexpr std::uint32_t kMaxGlyphSide     = 1024;
}

core::Result<Font> Font::parse(std::span<const std::byte> data) {
    core::ByteReader reader{data, "font"};
    Font font;

    const std::uint16_t version = reader.u16();
    font.first_char_            = reader.u8();
    font.point_size_            = reader.u8();
    static_cast<void>(reader.u8());
    static_cast<void>(reader.u8());
    const std::size_t count = reader.bounded_count(reader.u8(), 4 * 4);
    if (reader.ok() && version != kSupportedVersion)
        return core::fail(core::ErrorCode::unsupported, std::format("font version {}", version));
    if (reader.ok() && count == 0)
        reader.fail(core::ErrorCode::corrupt, "no glyphs");

    font.glyphs_.reserve(count);
    for (std::size_t i = 0; i < count && reader.ok(); ++i) {
        Glyph g;
        const std::uint32_t w = reader.u32();
        const std::uint32_t h = reader.u32();
        g.left_bearing_       = reader.i32();
        g.advance_pad_        = reader.i32();
        if (w > kMaxGlyphSide || h > kMaxGlyphSide) {
            reader.fail(core::ErrorCode::corrupt, std::format("glyph {} is {}x{}", i, w, h));
            break;
        }
        g.width_  = static_cast<std::int32_t>(w);
        g.height_ = static_cast<std::int32_t>(h);
        g.pixels_ = reader.u8_bytes(std::size_t{w} * h);
        font.glyphs_.push_back(std::move(g));
    }
    return reader.finish(std::move(font));
}

const Glyph& Font::glyph(char c) const noexcept {
    const auto code         = static_cast<unsigned char>(c);
    const std::size_t index = code >= first_char_ ? static_cast<std::size_t>(code - first_char_) : glyphs_.size();
    return index < glyphs_.size() ? glyphs_[index] : glyphs_.front();
}

}
