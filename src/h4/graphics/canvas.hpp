#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "h4/core/geometry.hpp"
#include "h4/graphics/color.hpp"
#include "h4/graphics/color565.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/graphics/text.hpp"

namespace h4::formats {
class Layer;
class Font;
}

namespace h4::graphics {

class Canvas {
public:
    explicit Canvas(Surface565& target) noexcept : target_{&target}, clip_{target.bounds()} {
    }

    [[nodiscard]] Canvas child(const core::Rect& local_rect) const noexcept;

    [[nodiscard]] Canvas translated(core::Point delta) const noexcept;

    [[nodiscard]] Canvas clipped_to(const core::Rect& local_rect) const noexcept {
        return child(local_rect).translated(core::Point{} - local_rect.origin());
    }

    [[nodiscard]] core::Rect local_clip() const noexcept {
        return clip_.translated(core::Point{} - origin_);
    }

    void fill_rect(const core::Rect& local, Rgba color) const noexcept;
    void stroke_rect(const core::Rect& local, Rgba color) const noexcept;

    void draw_layer(const formats::Layer& layer, core::Point offset = {}) const noexcept;
    void draw_layer_scaled(const formats::Layer& layer, core::Point offset, double scale) const noexcept;

    void draw_surface(const Surface565& source, core::Point at) const noexcept;

    void set_pixel(core::Point local, Color565 color) const noexcept;

    void blend_pixel(core::Point local, Color565 color, std::uint32_t weight) const noexcept {
        blend(local, color, weight);
    }

    void draw_text_line(const formats::Font& font, std::string_view text, core::Point at, const TextStyle& style) const noexcept;

    void draw_text_block(const formats::Font& font, std::string_view text, const core::Rect& local_box, const TextStyle& style, TextAlignment alignment) const;
    void draw_text_lines(const formats::Font& font,
                         std::span<const std::string> lines,
                         const core::Rect& local_box,
                         const TextStyle& style,
                         TextAlignment alignment) const;

private:
    Canvas(Surface565* target, core::Point origin, core::Rect clip) noexcept
    : target_{target}, origin_{origin}, clip_{clip} {
    }

    void blend(core::Point local, Color565 src, std::uint32_t weight) const noexcept;

    Surface565* target_;
    core::Point origin_{};
    core::Rect clip_;
};

}
