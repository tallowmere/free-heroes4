#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/graphics/color.hpp"

namespace h4::formats {
class Font;
}

namespace h4::graphics {

struct TextStyle {
    Rgba body{colors::black};
    std::optional<Rgba> shadow;

    [[nodiscard]] static constexpr TextStyle engraved() noexcept {
        return {colors::black, Rgba{200, 200, 200, 255}};
    }
};

enum class HorizontalAlign : std::uint8_t { left, centre, right };
enum class VerticalAlign : std::uint8_t { top, centre };

struct TextAlignment {
    HorizontalAlign horizontal = HorizontalAlign::left;
    VerticalAlign vertical     = VerticalAlign::top;
};

[[nodiscard]] std::int32_t measure_text(const formats::Font& font, std::string_view text) noexcept;
[[nodiscard]] std::int32_t measure_widest(const formats::Font& font, std::span<const std::string> lines) noexcept;

[[nodiscard]] core::Size wrapped_size(const formats::Font& font, std::span<const std::string> lines) noexcept;

[[nodiscard]] std::vector<std::string> wrap_text(const formats::Font& font, std::string_view text, std::int32_t max_width);

class WrappedText final {
public:
    [[nodiscard]] std::span<const std::string> lines(const formats::Font& font, std::string_view text, std::int32_t max_width);

private:
    const formats::Font* font_ = nullptr;
    std::string text_;
    std::int32_t width_ = 0;
    std::vector<std::string> lines_;
};

}
