#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <utility>

namespace h4::assets {

enum class AssetKind : std::uint8_t {
    group,
    terrain,
    object,
    font,
    transitions,
};

struct AssetRequest {
    AssetKind kind = AssetKind::group;
    std::string name;
    std::int32_t size = 0;

    [[nodiscard]] static AssetRequest group(std::string name) {
        return {AssetKind::group, std::move(name), 0};
    }

    [[nodiscard]] static AssetRequest object(std::string name) {
        return {AssetKind::object, std::move(name), 0};
    }

    [[nodiscard]] static AssetRequest terrain(std::string name) {
        return {AssetKind::terrain, std::move(name), 0};
    }

    [[nodiscard]] static AssetRequest transitions() {
        return {AssetKind::transitions, {}, 0};
    }

    [[nodiscard]] static AssetRequest font(std::int32_t size) {
        return {AssetKind::font, {}, size};
    }

    [[nodiscard]] static AssetRequest every_font() {
        return font(0);
    }

    friend auto operator<=>(const AssetRequest&, const AssetRequest&) = default;
};

}
