#include "h4/ui/widgets/text_window.hpp"

#include <algorithm>

#include "h4/graphics/text.hpp"

namespace h4::ui {

namespace {

constexpr std::int32_t kScrollMinWidth  = 100;
constexpr std::int32_t kScrollMinHeight = 40;

std::int32_t line_count(graphics::WrappedText& wrapped, const formats::Font& font, std::string_view text, std::int32_t width) {
    return static_cast<std::int32_t>(wrapped.lines(font, text, width).size());
}

}

TextWindow::TextWindow(core::Rect rect, std::string text, const formats::Font& font, LabelStyle style, std::optional<ScrollbarArt> scroll_art)
: Widget{rect}, text_{std::move(text)}, font_{&font}, style_{style}, art_{std::move(scroll_art)} {
    scroll_ = &emplace_child<ScrollView>(core::Rect{{}, rect.size()}, std::max(1, font.point_size()));
    label_  = &scroll_->sheet().emplace_child<Label>(core::Rect{{}, rect.size()}, text_, font, style_);
    reflow();
}

void TextWindow::set_text(std::string text) {
    text_ = std::move(text);
    label_->set_text(text_);
    reflow();
}

void TextWindow::reflow() {
    const core::Size size    = rect().size();
    const std::int32_t point = std::max(1, font_->point_size());
    scroll_->set_rect(core::Rect{{}, size});
    scroll_->detach();
    if (scrollbar_ != nullptr) {
        drop_child(*scrollbar_);
        scrollbar_ = nullptr;
    }
    scroll_->set_position(0);

    wrap_width_          = size.width;
    lines_               = line_count(wrapped_, *font_, text_, wrap_width_);
    const bool overflows = lines_ * point > size.height;
    if (!overflows || !art_ || size.width < kScrollMinWidth || size.height < kScrollMinHeight) {
        label_->set_style(style_);
        label_->set_rect(core::Rect{{}, {wrap_width_, std::max(size.height, lines_ * point)}});
        scroll_->set_content_height(std::max(size.height, lines_ * point));
        return;
    }

    wrap_width_               = size.width - art_->width();
    lines_                    = line_count(wrapped_, *font_, text_, wrap_width_);
    const std::int32_t height = std::max(size.height, lines_ * point);
    label_->set_style(LabelStyle{style_.ink, {style_.alignment.horizontal, graphics::VerticalAlign::top}});
    label_->set_rect(core::Rect{{}, {wrap_width_, height}});
    const std::int32_t overhang = std::max(0, lines_ - size.height / point);
    scroll_->set_content_height(size.height + overhang * point);
    scrollbar_ = &emplace_child<Scrollbar>(core::Point{wrap_width_, 0}, size.height, *art_, 0, overhang);
    scroll_->attach(*scrollbar_);
}

}
