#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace h4::model {

enum class Alignment : std::uint8_t { life, order, death, chaos, nature, might };

inline constexpr std::array<Alignment, 6> kAllAlignments{
    Alignment::life,
    Alignment::order,
    Alignment::death,
    Alignment::chaos,
    Alignment::nature,
    Alignment::might,
};

inline constexpr std::size_t kMagicAlignments = 5;
static_assert(std::to_underlying(Alignment::might) == kMagicAlignments);
[[nodiscard]] constexpr bool is_magic(Alignment a) noexcept {
    return std::to_underlying(a) < kMagicAlignments;
}

[[nodiscard]] constexpr std::optional<Alignment> alignment_of(std::int32_t index) noexcept {
    if (index < 0 || static_cast<std::size_t>(index) >= kAllAlignments.size())
        return std::nullopt;
    return kAllAlignments[static_cast<std::size_t>(index)];
}

[[nodiscard]] std::string_view to_string(Alignment a) noexcept;
[[nodiscard]] std::optional<Alignment> parse_alignment(std::string_view text) noexcept;

}
