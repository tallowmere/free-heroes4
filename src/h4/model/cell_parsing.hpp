#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace h4::model {

[[nodiscard]] inline std::optional<std::int32_t> parse_int(std::string_view cell) noexcept {
    while (!cell.empty() && cell.front() == ' ')
        cell.remove_prefix(1);
    while (!cell.empty() && cell.back() == ' ')
        cell.remove_suffix(1);
    if (cell.empty())
        return std::nullopt;
    std::int32_t value   = 0;
    const auto [end, ec] = std::from_chars(cell.data(), cell.data() + cell.size(), value);
    if (ec != std::errc{} || end != cell.data() + cell.size())
        return std::nullopt;
    return value;
}

[[nodiscard]] inline std::int32_t parse_int_or_zero(std::string_view cell) noexcept {
    return parse_int(cell).value_or(0);
}

}
