#include "h4/formats/bitmap_group.hpp"

#include <algorithm>
#include <format>
#include <ranges>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"

namespace h4::formats {

PaletteColor read_palette_color(core::ByteReader& in) {
    const std::uint8_t blue  = in.u8();
    const std::uint8_t green = in.u8();
    const std::uint8_t red   = in.u8();
    return {red, green, blue};
}

namespace {

constexpr std::int32_t kMaxDimension = 8192;

Transparency classify(std::uint8_t kind) noexcept {
    if (kind >= 4)
        return Transparency::alpha;
    if (kind == 1)
        return Transparency::color_keyed;
    return Transparency::opaque;
}

}

bool Layer::hit(core::Point p) const noexcept {
    if (is_placeholder() || !bounds_.contains(p))
        return false;
    if (transparency_ == Transparency::opaque)
        return true;
    const auto row = static_cast<std::size_t>(p.y - bounds_.top());
    if (row >= rows_.size())
        return false;
    const RowSpan& run   = rows_[row];
    const std::int32_t x = p.x - bounds_.left();
    if (x < run.x0 || x >= run.x1)
        return false;
    return coverage(run.offset + static_cast<std::size_t>(x - run.x0)) != 0;
}

std::uint8_t Layer::coverage(std::size_t i) const noexcept {
    if (i >= pixels_.size())
        return 0;
    switch (transparency_) {
    case Transparency::opaque: return 0x0F;
    case Transparency::color_keyed: return pixels_[i] == 0 ? 0 : 0x0F;
    case Transparency::alpha: {
        const std::uint8_t byte = alpha_[i / 2];
        return static_cast<std::uint8_t>((i & 1U) != 0 ? byte >> 4 : byte & 0x0F);
    }
    }
    return 0;
}

core::Result<BitmapGroup::Embedded> BitmapGroup::parse_embedded(std::span<const std::byte> data, std::size_t offset) {
    core::ByteReader reader{data, "bitmap group", offset};
    BitmapGroup group;

    const std::size_t count = reader.bounded_count(reader.u16(), 2 + 2 + 2 + 1 + 16);
    group.layers_.reserve(count);
    for (std::size_t index = 0; index < count && reader.ok(); ++index) {
        Layer layer;
        const std::size_t palette_size = reader.bounded_count(reader.u16(), 3);
        const bool has_opacity         = reader.u16() != 0;
        layer.palette_.reserve(palette_size);
        for (std::size_t p = 0; p < palette_size; ++p)
            layer.palette_.push_back(read_palette_color(reader));
        layer.name_               = std::string{reader.string16()};
        layer.transparency_       = classify(reader.u8());
        const std::int32_t left   = reader.i32();
        const std::int32_t top    = reader.i32();
        const std::int32_t right  = reader.i32();
        const std::int32_t bottom = reader.i32();
        if (!reader.ok())
            break;

        const std::int64_t width  = std::int64_t{right} - left;
        const std::int64_t height = std::int64_t{bottom} - top;
        if (height <= 0) {
            layer.bounds_ = core::Rect{{left, top}, {}};
            group.layers_.push_back(std::move(layer));
            continue;
        }
        if (width <= 0 || width > kMaxDimension || height > kMaxDimension) {
            reader.fail(core::ErrorCode::corrupt, std::format("layer {} has implausible size {}x{}", index, width, height));
            break;
        }
        if (left < -kMaxDimension || left > kMaxDimension || top < -kMaxDimension || top > kMaxDimension) {
            reader.fail(core::ErrorCode::corrupt,
                        std::format("layer {} has implausible origin ({}, "
                                    "{})",
                                    index,
                                    left,
                                    top));
            break;
        }
        layer.bounds_ = core::Rect::from_edges(left, top, right, bottom);

        const std::size_t row_count = reader.bounded_count(static_cast<std::size_t>(height), 8);
        layer.rows_.reserve(row_count);
        std::size_t run = 0;
        for (std::size_t y = 0; y < row_count && reader.ok(); ++y) {
            RowSpan span{reader.u16(), reader.u16(), reader.u32()};
            if (span.x1 < span.x0 || span.x1 > width || span.offset != run)
                reader.fail(core::ErrorCode::corrupt, std::format("layer {} row {} has an inconsistent span", index, y));
            run += span.length();
            layer.rows_.push_back(span);
        }

        const auto pixels = reader.bytes(run);
        layer.pixels_.reserve(pixels.size());
        for (const std::byte px : pixels) {
            const auto v = std::to_integer<std::uint8_t>(px);
            if (v >= palette_size) {
                reader.fail(core::ErrorCode::corrupt, std::format("layer {} indexes past its palette", index));
                break;
            }
            layer.pixels_.push_back(v);
        }

        if (layer.transparency_ == Transparency::alpha) {
            layer.alpha_ = reader.u8_bytes((run + 1) / 2);
            if (has_opacity)
                reader.skip((run + 131) / 128);
        }
        group.layers_.push_back(std::move(layer));
    }

    const std::size_t end = reader.position();
    return reader.finish(Embedded{std::move(group), end}, false);
}

core::Result<BitmapGroup> BitmapGroup::parse(std::span<const std::byte> data) {
    auto embedded = parse_embedded(data, 0);
    if (!embedded)
        return std::unexpected{embedded.error()};
    if (embedded->end != data.size())
        return core::fail(core::ErrorCode::corrupt,
                          std::format("bitmap group: {} trailing bytes", data.size() - embedded->end));
    return std::move(embedded->group);
}

const Layer* BitmapGroup::find(std::string_view name) const noexcept {
    const auto it = std::ranges::find_if(layers_ | std::views::reverse,
                                         [name](const Layer& l) { return core::iequals(l.name(), name); });
    return it == (layers_ | std::views::reverse).end() ? nullptr : &*it;
}

core::Rect BitmapGroup::bounds() const noexcept {
    core::Rect out;
    for (const Layer& l : layers_)
        if (!l.is_placeholder())
            out = out.united(l.bounds());
    return out;
}

}
