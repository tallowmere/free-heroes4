#pragma once

#include <algorithm>
#include <cstdint>

namespace h4::core {

struct Point {
    std::int32_t x = 0;
    std::int32_t y = 0;

    friend constexpr bool operator==(Point, Point) noexcept = default;

    [[nodiscard]] friend constexpr Point operator+(Point a, Point b) noexcept {
        return {a.x + b.x, a.y + b.y};
    }
    [[nodiscard]] friend constexpr Point operator-(Point a, Point b) noexcept {
        return {a.x - b.x, a.y - b.y};
    }
};

struct Size {
    std::int32_t width  = 0;
    std::int32_t height = 0;

    friend constexpr bool operator==(Size, Size) noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept {
        return width <= 0 || height <= 0;
    }
};

class Rect {
public:
    constexpr Rect() noexcept = default;

    constexpr Rect(Point origin, Size size) noexcept : origin_{origin}, size_{size} {
    }

    [[nodiscard]] static constexpr Rect from_edges(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept {
        return Rect{{left, top}, {right - left, bottom - top}};
    }

    [[nodiscard]] constexpr Point origin() const noexcept {
        return origin_;
    }
    [[nodiscard]] constexpr Size size() const noexcept {
        return size_;
    }
    [[nodiscard]] constexpr std::int32_t left() const noexcept {
        return origin_.x;
    }
    [[nodiscard]] constexpr std::int32_t top() const noexcept {
        return origin_.y;
    }
    [[nodiscard]] constexpr std::int32_t right() const noexcept {
        return origin_.x + size_.width;
    }
    [[nodiscard]] constexpr std::int32_t bottom() const noexcept {
        return origin_.y + size_.height;
    }
    [[nodiscard]] constexpr std::int32_t width() const noexcept {
        return size_.width;
    }
    [[nodiscard]] constexpr std::int32_t height() const noexcept {
        return size_.height;
    }
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_.empty();
    }

    [[nodiscard]] constexpr bool contains(Point p) const noexcept {
        return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
    }

    [[nodiscard]] constexpr Rect translated(Point delta) const noexcept {
        return Rect{origin_ + delta, size_};
    }

    [[nodiscard]] constexpr Rect intersected(const Rect& other) const noexcept {
        const std::int32_t l = std::max(left(), other.left());
        const std::int32_t t = std::max(top(), other.top());
        const std::int32_t r = std::min(right(), other.right());
        const std::int32_t b = std::min(bottom(), other.bottom());
        if (r <= l || b <= t)
            return Rect{{l, t}, {}};
        return from_edges(l, t, r, b);
    }

    [[nodiscard]] constexpr Rect united(const Rect& other) const noexcept {
        if (empty())
            return other;
        if (other.empty())
            return *this;
        return from_edges(std::min(left(), other.left()),
                          std::min(top(), other.top()),
                          std::max(right(), other.right()),
                          std::max(bottom(), other.bottom()));
    }

    [[nodiscard]] constexpr Point centre() const noexcept {
        return {left() + width() / 2, top() + height() / 2};
    }

    [[nodiscard]] constexpr Rect centred_in(const Rect& outer) const noexcept {
        return Rect{{outer.left() + (outer.width() - width()) / 2, outer.top() + (outer.height() - height()) / 2}, size_};
    }

    friend constexpr bool operator==(const Rect&, const Rect&) noexcept = default;

private:
    Point origin_{};
    Size size_{};
};

}
