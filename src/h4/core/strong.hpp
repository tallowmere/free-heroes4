#pragma once

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace h4::core {

struct Additive {};
struct Scalable {};

template <class Cap, class... Caps> inline constexpr bool has_capability = (std::is_same_v<Cap, Caps> || ...);

template <class Tag, class Rep, class... Caps>
    requires std::is_arithmetic_v<Rep>
class Strong {
public:
    using rep_type = Rep;
    using tag_type = Tag;

    constexpr Strong() noexcept = default;
    constexpr explicit Strong(Rep value) noexcept : value_{value} {
    }

    [[nodiscard]] constexpr Rep value() const noexcept {
        return value_;
    }

    friend constexpr bool operator==(Strong, Strong) noexcept  = default;
    friend constexpr auto operator<=>(Strong, Strong) noexcept = default;

    [[nodiscard]] friend constexpr Strong operator+(Strong a, Strong b) noexcept
        requires has_capability<Additive, Caps...>
    {
        return Strong{static_cast<Rep>(a.value_ + b.value_)};
    }
    [[nodiscard]] friend constexpr Strong operator-(Strong a, Strong b) noexcept
        requires has_capability<Additive, Caps...>
    {
        return Strong{static_cast<Rep>(a.value_ - b.value_)};
    }
    [[nodiscard]] friend constexpr Strong operator-(Strong a) noexcept
        requires has_capability<Additive, Caps...>
    {
        return Strong{static_cast<Rep>(-a.value_)};
    }
    constexpr Strong& operator+=(Strong other) noexcept
        requires has_capability<Additive, Caps...>
    {
        value_ = static_cast<Rep>(value_ + other.value_);
        return *this;
    }
    constexpr Strong& operator-=(Strong other) noexcept
        requires has_capability<Additive, Caps...>
    {
        value_ = static_cast<Rep>(value_ - other.value_);
        return *this;
    }

    [[nodiscard]] friend constexpr Strong operator*(Strong a, Rep n) noexcept
        requires has_capability<Scalable, Caps...>
    {
        return Strong{static_cast<Rep>(a.value_ * n)};
    }
    [[nodiscard]] friend constexpr Strong operator*(Rep n, Strong a) noexcept
        requires has_capability<Scalable, Caps...>
    {
        return Strong{static_cast<Rep>(a.value_ * n)};
    }
    [[nodiscard]] friend constexpr Strong operator/(Strong a, Rep n) noexcept
        requires has_capability<Scalable, Caps...>
    {
        return Strong{static_cast<Rep>(a.value_ / n)};
    }

private:
    Rep value_{};
};

}

template <class Tag, class Rep, class... Caps> struct std::hash<h4::core::Strong<Tag, Rep, Caps...>> {
    [[nodiscard]] std::size_t operator()(h4::core::Strong<Tag, Rep, Caps...> v) const noexcept {
        return std::hash<Rep>{}(v.value());
    }
};
