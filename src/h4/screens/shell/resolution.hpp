#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"

namespace h4::screens {

enum class Resolution : std::uint8_t { r800x600, r1024x768, r1280x1024 };

inline constexpr std::array<Resolution, 3> kAllResolutions{Resolution::r800x600, Resolution::r1024x768, Resolution::r1280x1024};

[[nodiscard]] constexpr core::Size size_of(Resolution r) noexcept {
    switch (r) {
    case Resolution::r800x600: return {800, 600};
    case Resolution::r1024x768: return {1024, 768};
    case Resolution::r1280x1024: return {1280, 1024};
    }
    return {};
}

[[nodiscard]] constexpr std::string_view variant_suffix(Resolution r) noexcept {
    switch (r) {
    case Resolution::r800x600: return "0800";
    case Resolution::r1024x768: return "1024";
    case Resolution::r1280x1024: return "1280";
    }
    return {};
}

[[nodiscard]] constexpr std::optional<Resolution> resolution_for_width(std::int32_t width) noexcept {
    for (const Resolution r : kAllResolutions)
        if (size_of(r).width == width)
            return r;
    return std::nullopt;
}

[[nodiscard]] constexpr Resolution variant_for_height(std::int32_t height) noexcept {
    Resolution picked = Resolution::r800x600;
    for (const Resolution r : kAllResolutions)
        if (size_of(r).height <= height)
            picked = r;
    return picked;
}

[[nodiscard]] constexpr core::Size logical_frame(core::Size window) noexcept {
    const core::Size base = size_of(variant_for_height(window.height));
    if (window.height <= 0 || window.width <= 0)
        return base;
    const std::int32_t width = static_cast<std::int32_t>(
        (static_cast<std::int64_t>(window.width) * base.height + window.height / 2) / window.height
    );
    return {std::max(base.width, width), base.height};
}

namespace resolution {

[[nodiscard]] inline std::optional<core::Size> pick(const std::vector<core::Size>& available, core::Size current) noexcept {
    std::optional<core::Size> best;
    std::optional<core::Size> fallback;
    for (core::Size candidate : available) {
        const bool fits = candidate.width <= current.width && candidate.height <= current.height;
        if (fits) {
            if (!best || candidate.width > best->width || candidate.height > best->height)
                best = candidate;
        } else if (!fallback || candidate.width < fallback->width || candidate.height < fallback->height) {
            fallback = candidate;
        }
    }
    return best ? best : fallback;
}
}

}
