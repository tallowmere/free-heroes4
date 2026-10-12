#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace h4::assets {

struct LadderStep {
    std::int32_t asked;
    std::string_view entry;
};

inline constexpr std::array<LadderStep, 15> kFontLadder{{
    {9, "font.Small Fonts5.h4d"},
    {11, "font.Small Fonts6.h4d"},
    {12, "font.Prose_Antique.12.h4d"},
    {14, "font.Prose_Antique.14.h4d"},
    {16, "font.Prose_Antique.16.h4d"},
    {18, "font.Prose_Antique.18.h4d"},
    {20, "font.Prose_Antique.20.h4d"},
    {23, "font.Prose_Antique.22.h4d"},
    {25, "font.Prose_Antique.24.h4d"},
    {27, "font.Prose_Antique.26.h4d"},
    {29, "font.Prose_Antique.28.h4d"},
    {30, "font.Prose_Antique.30.h4d"},
    {33, "font.Prose_Antique.32.h4d"},
    {34, "font.Prose_Antique.34.h4d"},
    {36, "font.Prose_Antique.36.h4d"},
}};

[[nodiscard]] constexpr std::size_t ladder_index(std::int32_t asked) noexcept {
    std::size_t pick = 0;
    for (std::size_t i = 0; i < kFontLadder.size(); ++i)
        if (kFontLadder[i].asked <= asked)
            pick = i;
    return pick;
}

inline std::string entry_name(std::string_view prefix, std::string_view name) {
    return std::format("{}.{}.h4d", prefix, name);
}

template <class T, class Source> std::unique_ptr<const T> decode(const Source& source, const std::string& entry) {
    auto bytes = source.read(entry);
    if (!bytes)
        return nullptr;
    auto parsed = T::parse(*bytes);
    return parsed ? std::make_unique<const T>(std::move(*parsed)) : nullptr;
}

}
