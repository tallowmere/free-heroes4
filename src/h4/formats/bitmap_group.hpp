#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

struct PaletteColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    friend constexpr bool operator==(PaletteColor, PaletteColor) noexcept = default;
};

[[nodiscard]] PaletteColor read_palette_color(core::ByteReader& in);

struct RowSpan {
    std::uint16_t x0     = 0;
    std::uint16_t x1     = 0;
    std::uint32_t offset = 0;

    [[nodiscard]] constexpr std::size_t length() const noexcept {
        return static_cast<std::size_t>(x1 - x0);
    }
};

enum class Transparency : std::uint8_t {
    opaque,
    color_keyed,
    alpha,
};

class Layer {
public:
    [[nodiscard]] std::string_view name() const noexcept {
        return name_;
    }
    [[nodiscard]] core::Rect bounds() const noexcept {
        return bounds_;
    }
    [[nodiscard]] Transparency transparency() const noexcept {
        return transparency_;
    }

    [[nodiscard]] bool is_placeholder() const noexcept {
        return rows_.empty();
    }

    [[nodiscard]] std::span<const PaletteColor> palette() const noexcept {
        return palette_;
    }
    [[nodiscard]] std::span<const RowSpan> rows() const noexcept {
        return rows_;
    }
    [[nodiscard]] std::span<const std::uint8_t> pixels() const noexcept {
        return pixels_;
    }

    [[nodiscard]] std::uint8_t coverage(std::size_t i) const noexcept;

    [[nodiscard]] bool hit(core::Point p) const noexcept;

    [[nodiscard]] std::size_t ink() const noexcept {
        return pixels_.size();
    }

private:
    friend class BitmapGroup;
    Layer() = default;

    std::string name_;
    core::Rect bounds_;
    Transparency transparency_ = Transparency::opaque;
    std::vector<PaletteColor> palette_;
    std::vector<RowSpan> rows_;
    std::vector<std::uint8_t> pixels_;
    std::vector<std::uint8_t> alpha_;
};

class BitmapGroup {
public:
    [[nodiscard]] static core::Result<BitmapGroup> parse(std::span<const std::byte> data);

    struct Embedded;
    [[nodiscard]] static core::Result<Embedded> parse_embedded(std::span<const std::byte> data, std::size_t offset);

    [[nodiscard]] std::span<const Layer> layers() const noexcept {
        return layers_;
    }

    [[nodiscard]] const Layer* find(std::string_view name) const noexcept;

    [[nodiscard]] core::Rect bounds() const noexcept;

private:
    std::vector<Layer> layers_;
};

struct BitmapGroup::Embedded {
    BitmapGroup group;
    std::size_t end = 0;
};

}
