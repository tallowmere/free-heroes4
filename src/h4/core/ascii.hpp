#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace h4::core {

[[nodiscard]] constexpr char ascii_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

[[nodiscard]] inline std::string ascii_lowered(std::string_view s) {
    std::string out{s};
    for (char& c : out)
        c = ascii_lower(c);
    return out;
}

[[nodiscard]] constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (ascii_lower(a[i]) != ascii_lower(b[i]))
            return false;
    return true;
}

[[nodiscard]] constexpr bool istarts_with(std::string_view s, std::string_view prefix) noexcept {
    return s.size() >= prefix.size() && iequals(s.substr(0, prefix.size()), prefix);
}

[[nodiscard]] constexpr bool iends_with(std::string_view s, std::string_view suffix) noexcept {
    return s.size() >= suffix.size() && iequals(s.substr(s.size() - suffix.size()), suffix);
}

[[nodiscard]] constexpr bool icontains(std::string_view haystack, std::string_view needle) noexcept {
    for (std::size_t at = 0; at + needle.size() <= haystack.size(); ++at)
        if (iequals(haystack.substr(at, needle.size()), needle))
            return true;
    return false;
}

struct CaseInsensitiveHash {
    using is_transparent = void;
    [[nodiscard]] constexpr std::size_t operator()(std::string_view s) const noexcept {
        std::size_t h = 1469598103934665603ULL;
        for (char c : s) {
            h ^= static_cast<unsigned char>(ascii_lower(c));
            h *= 1099511628211ULL;
        }
        return h;
    }
};

struct CaseInsensitiveEqual {
    using is_transparent = void;
    [[nodiscard]] constexpr bool operator()(std::string_view a, std::string_view b) const noexcept {
        return iequals(a, b);
    }
};

template <class V> using FoldedMap = std::unordered_map<std::string, V, CaseInsensitiveHash, CaseInsensitiveEqual>;
using FoldedSet                    = std::unordered_set<std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

[[nodiscard]] constexpr std::optional<std::size_t> index_of(std::span<const std::string_view> table, std::string_view name) noexcept {
    for (std::size_t i = 0; i < table.size(); ++i)
        if (iequals(table[i], name))
            return i;
    return std::nullopt;
}

}
