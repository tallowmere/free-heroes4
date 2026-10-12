#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "h4/formats/font.hpp"
#include "h4/graphics/text.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/scroll_view.hpp"
#include "h4/ui/widgets/scrollbar.hpp"

namespace h4::ui {

class TextWindow final : public Widget {
public:
    TextWindow(core::Rect rect, std::string text, const formats::Font& font, LabelStyle style, std::optional<ScrollbarArt> scroll_art);

    [[nodiscard]] std::string_view text() const noexcept {
        return text_;
    }
    void set_text(std::string text);

    void set_rect(core::Rect rect) {
        Widget::set_rect(rect);
        reflow();
    }

    [[nodiscard]] const Scrollbar* scrollbar() const noexcept {
        return scrollbar_;
    }
    [[nodiscard]] Scrollbar* scrollbar() noexcept {
        return scrollbar_;
    }
    [[nodiscard]] std::int32_t first_line() const noexcept {
        return scroll_->position();
    }
    [[nodiscard]] std::int32_t wrap_width() const noexcept {
        return wrap_width_;
    }

private:
    void reflow();

    std::string text_;
    const formats::Font* font_;
    LabelStyle style_;
    std::optional<ScrollbarArt> art_;
    ScrollView* scroll_      = nullptr;
    Label* label_            = nullptr;
    Scrollbar* scrollbar_    = nullptr;
    std::int32_t wrap_width_ = 0;
    std::int32_t lines_      = 0;
    graphics::WrappedText wrapped_;
};

}
