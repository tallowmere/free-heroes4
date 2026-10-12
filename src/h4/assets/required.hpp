#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <utility>

#include "h4/core/error.hpp"

namespace h4::formats {
class BitmapGroup;
class Font;
class Layer;
}

namespace h4::assets {

class Assets;

[[nodiscard]] core::Result<const formats::BitmapGroup*> require_group(const Assets& assets, std::string_view name);
[[nodiscard]] core::Result<const formats::Font*> require_font(const Assets& assets, std::int32_t size);
[[nodiscard]] core::Result<const formats::Layer*>
require_layer(const formats::BitmapGroup& group, std::string_view name, std::string_view layer);
[[nodiscard]] core::Result<const formats::Layer*> require_layer(const Assets& assets, std::string_view name, std::string_view layer);
[[nodiscard]] core::Result<void> require_layers(const formats::BitmapGroup& group,
                                                std::string_view name,
                                                std::span<const std::string_view> layers,
                                                std::span<const formats::Layer*> out);
template <std::size_t N>
[[nodiscard]] core::Result<std::array<const formats::Layer*, N>>
require_layers(const formats::BitmapGroup& group, std::string_view name, const std::string_view (&layers)[N]) {
    std::array<const formats::Layer*, N> out{};
    if (auto found = require_layers(group, name, layers, out); !found)
        return std::unexpected{found.error()};
    return out;
}
template <std::size_t N>
[[nodiscard]] core::Result<std::array<const formats::Layer*, N>>
require_layers(const Assets& assets, std::string_view name, const std::string_view (&layers)[N]) {
    const auto group = require_group(assets, name);
    if (!group)
        return std::unexpected{group.error()};
    return require_layers(**group, name, layers);
}

template <typename Art, typename... Args>
[[nodiscard]] core::Result<Art> require_art(const Assets& assets, std::string_view name, Args&&... args) {
    const auto group = require_group(assets, name);
    if (!group)
        return std::unexpected{group.error()};
    auto art = Art::from_group(**group, std::forward<Args>(args)...);
    if (!art)
        return core::fail(core::ErrorCode::corrupt, std::format("layers.{} is incomplete", name));
    return std::move(*art);
}

}
