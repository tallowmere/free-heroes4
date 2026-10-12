#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/graphics/color565.hpp"

namespace h4::formats {
class RawBitmap;
}

namespace h4::graphics {

class Surface565 {
public:
    Surface565() = default;
    explicit Surface565(core::Size size, Color565 fill = {});

    [[nodiscard]] static Surface565 from_raw_bitmap(const formats::RawBitmap& bitmap);

    [[nodiscard]] core::Size size() const noexcept {
        return size_;
    }
    [[nodiscard]] core::Rect bounds() const noexcept {
        return {{}, size_};
    }
    [[nodiscard]] bool empty() const noexcept {
        return size_.empty();
    }

    [[nodiscard]] std::span<const std::uint16_t> bits() const noexcept {
        return bits_;
    }

    [[nodiscard]] Color565 at(core::Point p) const noexcept;
    void set(core::Point p, Color565 c) noexcept;
    void fill(Color565 c) noexcept;

    [[nodiscard]] std::vector<std::uint8_t> to_rgba() const;

private:
    [[nodiscard]] std::size_t index(core::Point p) const noexcept {
        return static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x);
    }

    core::Size size_;
    std::vector<std::uint16_t> bits_;
};

}
