#include "h4/graphics/canvas.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/graphics/nearest.hpp"

namespace h4::graphics {

namespace {

std::array<Color565, 256> palette565(const formats::Layer& layer) noexcept {
    std::array<Color565, 256> palette{};
    const auto source = layer.palette();
    for (std::size_t i = 0; i < std::min<std::size_t>(source.size(), palette.size()); ++i)
        palette[i] = Color565::from_rgb(source[i].r, source[i].g, source[i].b);
    return palette;
}

}

Canvas Canvas::child(const core::Rect& local_rect) const noexcept {
    const core::Rect in_frame = local_rect.translated(origin_);
    return Canvas{target_, in_frame.origin(), clip_.intersected(in_frame)};
}

Canvas Canvas::translated(core::Point delta) const noexcept {
    return Canvas{target_, origin_ + delta, clip_};
}

void Canvas::blend(core::Point local, Color565 src, std::uint32_t weight) const noexcept {
    const core::Point p = local + origin_;
    if (clip_.contains(p))
        target_->set(p, lerp(target_->at(p), src, weight));
}

void Canvas::fill_rect(const core::Rect& local, Rgba color) const noexcept {
    const std::uint32_t weight = color.a * std::uint32_t{16} / 255;
    if (weight == 0)
        return;
    const Color565 src = Color565::from(color);
    const core::Rect r = local.translated(origin_).intersected(clip_);
    for (std::int32_t y = r.top(); y < r.bottom(); ++y)
        for (std::int32_t x = r.left(); x < r.right(); ++x)
            target_->set({x, y}, lerp(target_->at({x, y}), src, weight));
}

void Canvas::stroke_rect(const core::Rect& local, Rgba color) const noexcept {
    if (local.empty())
        return;
    fill_rect({local.origin(), {local.width(), 1}}, color);
    fill_rect({{local.left(), local.bottom() - 1}, {local.width(), 1}}, color);
    fill_rect({local.origin(), {1, local.height()}}, color);
    fill_rect({{local.right() - 1, local.top()}, {1, local.height()}}, color);
}

void Canvas::draw_layer(const formats::Layer& layer, core::Point offset) const noexcept {
    if (layer.is_placeholder())
        return;
    const core::Rect placed = layer.bounds().translated(offset + origin_);
    if (placed.intersected(clip_).empty())
        return;

    const std::array<Color565, 256> palette = palette565(layer);

    const auto pixels = layer.pixels();
    const auto rows   = layer.rows();
    for (std::size_t y = 0; y < rows.size(); ++y) {
        const std::int32_t py = placed.top() + static_cast<std::int32_t>(y);
        if (py < clip_.top() || py >= clip_.bottom())
            continue;
        const formats::RowSpan& run = rows[y];
        for (std::size_t i = 0; i < run.length(); ++i) {
            const std::size_t index  = run.offset + i;
            const std::uint8_t cover = layer.coverage(index);
            if (cover == 0)
                continue;
            const core::Point p{placed.left() + run.x0 + static_cast<std::int32_t>(i), py};
            if (!clip_.contains(p))
                continue;
            target_->set(p, lerp(target_->at(p), palette[pixels[index]], coverage_weight(cover)));
        }
    }
}

void Canvas::draw_layer_scaled(const formats::Layer& layer, core::Point offset, double scale) const noexcept {
    if (layer.is_placeholder() || scale <= 0.0)
        return;
    const core::Rect art      = layer.bounds();
    const std::int32_t left   = static_cast<std::int32_t>(std::floor(art.left() * scale));
    const std::int32_t top    = static_cast<std::int32_t>(std::floor(art.top() * scale));
    const std::int32_t right  = static_cast<std::int32_t>(std::ceil(art.right() * scale));
    const std::int32_t bottom = static_cast<std::int32_t>(std::ceil(art.bottom() * scale));
    const core::Point shift   = offset + origin_;

    const std::array<Color565, 256> palette = palette565(layer);
    const auto pixels                       = layer.pixels();
    const auto rows                         = layer.rows();
    for (std::int32_t y = top; y < bottom; ++y) {
        const std::int32_t py = y + shift.y;
        if (py < clip_.top() || py >= clip_.bottom())
            continue;
        const std::int32_t row = nearest_source(y, scale) - art.top();
        if (row < 0 || row >= static_cast<std::int32_t>(rows.size()))
            continue;
        const formats::RowSpan& run = rows[static_cast<std::size_t>(row)];
        for (std::int32_t x = left; x < right; ++x) {
            const core::Point p{x + shift.x, py};
            if (!clip_.contains(p))
                continue;
            const std::int32_t column = nearest_source(x, scale) - art.left();
            if (column < run.x0 || column >= run.x1)
                continue;
            const std::size_t index  = run.offset + static_cast<std::size_t>(column - run.x0);
            const std::uint8_t cover = layer.coverage(index);
            if (cover == 0)
                continue;
            target_->set(p, lerp(target_->at(p), palette[pixels[index]], coverage_weight(cover)));
        }
    }
}

void Canvas::draw_surface(const Surface565& source, core::Point at) const noexcept {
    const core::Point shift = at + origin_;
    const core::Rect placed = source.bounds().translated(shift).intersected(clip_);
    for (std::int32_t y = placed.top(); y < placed.bottom(); ++y)
        for (std::int32_t x = placed.left(); x < placed.right(); ++x)
            target_->set({x, y}, source.at(core::Point{x, y} - shift));
}

void Canvas::set_pixel(core::Point local, Color565 color) const noexcept {
    const core::Point at{local.x + origin_.x, local.y + origin_.y};
    if (clip_.contains(at))
        target_->set(at, color);
}

void Canvas::draw_text_line(const formats::Font& font, std::string_view text, core::Point at, const TextStyle& style) const noexcept {
    const Color565 body                  = Color565::from(style.body);
    const std::optional<Color565> shadow = style.shadow ? std::optional{Color565::from(*style.shadow)} : std::nullopt;
    std::int32_t pen                     = at.x;
    bool first                           = true;
    for (const char ch : text) {
        const formats::Glyph& g = font.glyph(ch);
        if (!first)
            pen += g.left_bearing();
        first = false;
        for (std::int32_t gy = 0; gy < g.height(); ++gy) {
            for (std::int32_t gx = 0; gx < g.width(); ++gx) {
                const core::Point p{pen + gx, at.y + gy};
                if (shadow)
                    if (const auto n = g.shadow(gx, gy); n != 0)
                        blend(p, *shadow, coverage_weight(n));
                if (const auto n = g.body(gx, gy); n != 0)
                    blend(p, body, coverage_weight(n));
            }
        }
        pen += g.width() + g.advance_pad();
    }
}

void Canvas::draw_text_block(const formats::Font& font, std::string_view text, const core::Rect& box, const TextStyle& style, TextAlignment alignment) const {
    draw_text_lines(font, wrap_text(font, text, box.width()), box, style, alignment);
}

void Canvas::draw_text_lines(const formats::Font& font,
                             std::span<const std::string> lines,
                             const core::Rect& box,
                             const TextStyle& style,
                             TextAlignment alignment) const {
    const Canvas clipped         = child(box);
    const std::int32_t line_step = font.point_size();
    const std::int32_t block_h   = line_step * static_cast<std::int32_t>(lines.size());
    std::int32_t y               = 0;
    if (alignment.vertical == VerticalAlign::centre && block_h < box.height())
        y = (box.height() - block_h) / 2;

    for (const std::string& line : lines) {
        if (y >= box.height())
            break;
        const std::int32_t w = measure_text(font, line);
        std::int32_t x       = 0;
        switch (alignment.horizontal) {
        case HorizontalAlign::left: break;
        case HorizontalAlign::centre: x = (box.width() - w) / 2; break;
        case HorizontalAlign::right: x = box.width() - w; break;
        }
        clipped.draw_text_line(font, line, {std::max(0, x), y}, style);
        y += line_step;
    }
}

}
