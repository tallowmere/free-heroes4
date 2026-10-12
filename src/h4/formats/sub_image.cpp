#include "h4/formats/sub_image.hpp"

#include <algorithm>
#include <charconv>
#include <span>
#include <string_view>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::formats {

namespace {

void add_numbers(std::vector<FrameGroup>& groups, std::string_view numbers, std::uint32_t layer) {
    std::size_t at = 0;
    while (at < numbers.size()) {
        while (at < numbers.size() && numbers[at] == ' ')
            ++at;
        const std::size_t begin = at;
        while (at < numbers.size() && numbers[at] >= '0' && numbers[at] <= '9')
            ++at;
        if (at == begin)
            break;
        std::uint32_t value = 0;
        std::from_chars(numbers.data() + begin, numbers.data() + at, value);
        const auto it = std::ranges::find(groups, value, &FrameGroup::number);
        if (it != groups.end())
            it->layers.push_back(layer);
        else
            groups.push_back({value, {layer}});
    }
}

}

SubImageLayers classify_sub_image(const BitmapGroup& group) {
    SubImageLayers out;
    const std::span<const Layer> layers = group.layers();
    for (std::uint32_t i = 0; i < layers.size(); ++i) {
        const std::string_view name = layers[i].name();
        if (core::istarts_with(name, "base_frame"))
            out.base_frame = static_cast<std::int32_t>(i);
        else if (core::istarts_with(name, "base_shadow"))
            out.base_shadow = static_cast<std::int32_t>(i);
        else if (core::istarts_with(name, "frame "))
            add_numbers(out.frames, name.substr(6), i);
        else if (core::istarts_with(name, "shadow "))
            add_numbers(out.shadows, name.substr(7), i);
    }
    std::ranges::sort(out.frames, {}, &FrameGroup::number);
    std::ranges::sort(out.shadows, {}, &FrameGroup::number);
    return out;
}

core::Result<SubImageBody> read_sub_image_body(std::span<const std::byte> payload, core::ByteReader& in, bool v1_name) {
    auto embedded = BitmapGroup::parse_embedded(payload, in.position());
    if (!embedded)
        return std::unexpected{embedded.error()};
    in.seek(embedded->end);
    SubImageBody out{std::move(embedded->group), {}};
    out.anchor = {in.i32(), in.i32()};
    if (v1_name)
        (void)in.string16();
    return out;
}

core::Rect frame_extent(const BitmapGroup& group, const SubImageLayers& layers, bool shadows) {
    const std::span<const Layer> all = group.layers();
    core::Rect out;
    const auto add = [&](std::int64_t index) {
        if (index < 0 || static_cast<std::size_t>(index) >= all.size())
            return;
        const Layer& layer = all[static_cast<std::size_t>(index)];
        if (!layer.is_placeholder())
            out = out.united(layer.bounds());
    };
    add(layers.base_frame);
    if (shadows)
        add(layers.base_shadow);
    for (const FrameGroup& g : layers.frames)
        for (const std::uint32_t layer : g.layers)
            add(layer);
    if (shadows)
        for (const FrameGroup& g : layers.shadows)
            for (const std::uint32_t layer : g.layers)
                add(layer);
    return out;
}

}
