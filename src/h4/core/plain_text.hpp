#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string_view>

namespace h4::core {

[[nodiscard]] constexpr std::string_view trimmed(std::string_view s) noexcept {
    const std::size_t first = s.find_first_not_of(" \t\r");
    if (first == std::string_view::npos)
        return {};
    return s.substr(first, s.find_last_not_of(" \t\r") - first + 1);
}

[[nodiscard]] constexpr std::string_view take_line(std::string_view& text) noexcept {
    const std::size_t end       = text.find('\n');
    const std::string_view line = trimmed(text.substr(0, end));
    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
    return line;
}

template <std::integral T> [[nodiscard]] std::optional<T> whole_number(std::string_view text) noexcept {
    T value{};
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size())
        return std::nullopt;
    return value;
}

}
