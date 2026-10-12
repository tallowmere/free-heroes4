#pragma once

#include <string>

#include "h4/formats/font.hpp"
#include "h4/graphics/text.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

struct LabelStyle {
    graphics::TextStyle ink           = graphics::TextStyle::engraved();
    graphics::TextAlignment alignment = {};
};

inline constexpr LabelStyle kEngravedLeft{graphics::TextStyle::engraved(),
                                          {graphics::HorizontalAlign::left, graphics::VerticalAlign::centre}};
inline constexpr LabelStyle kEngravedCentred{graphics::TextStyle::engraved(),
                                             {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre}};
inline constexpr LabelStyle kEngravedRight{graphics::TextStyle::engraved(),
                                           {graphics::HorizontalAlign::right, graphics::VerticalAlign::centre}};

class Label final : public Widget {
public:
    Label(core::Rect rect, std::string text, const formats::Font& font, LabelStyle style = {})
    : Widget{rect}, text_{std::move(text)}, font_{&font}, style_{style} {
    }

    [[nodiscard]] std::string_view text() const noexcept {
        return text_;
    }
    void set_text(std::string text) {
        text_ = std::move(text);
    }
    void set_style(LabelStyle style) {
        style_ = style;
    }

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    std::string text_;
    const formats::Font* font_;
    LabelStyle style_;
    mutable graphics::WrappedText wrapped_;
};

}
