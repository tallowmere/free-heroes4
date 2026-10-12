#include "h4/assets/required.hpp"

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::assets {

core::Result<const formats::BitmapGroup*> require_group(const Assets& assets, std::string_view name) {
    const formats::BitmapGroup* group = assets.group(name);
    if (group == nullptr)
        return core::fail(core::ErrorCode::not_found, std::format("layers.{} missing", name));
    return group;
}

core::Result<const formats::Font*> require_font(const Assets& assets, std::int32_t size) {
    const formats::Font* font = assets.font_for_size(size);
    if (font == nullptr)
        return core::fail(core::ErrorCode::not_found, std::format("no font for size {}", size));
    return font;
}

core::Result<const formats::Layer*> require_layer(const formats::BitmapGroup& group, std::string_view name, std::string_view layer) {
    const formats::Layer* out = group.find(layer);
    if (out == nullptr)
        return core::fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' layer", name, layer));
    return out;
}

core::Result<const formats::Layer*> require_layer(const Assets& assets, std::string_view name, std::string_view layer) {
    const auto group = require_group(assets, name);
    if (!group)
        return std::unexpected{group.error()};
    return require_layer(**group, name, layer);
}

core::Result<void> require_layers(const formats::BitmapGroup& group,
                                  std::string_view name,
                                  std::span<const std::string_view> layers,
                                  std::span<const formats::Layer*> out) {
    for (std::size_t i = 0; i < layers.size() && i < out.size(); ++i) {
        auto layer = require_layer(group, name, layers[i]);
        if (!layer)
            return std::unexpected{layer.error()};
        out[i] = *layer;
    }
    return {};
}

}
