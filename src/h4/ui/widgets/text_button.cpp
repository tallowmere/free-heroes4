#include "h4/ui/widgets/text_button.hpp"

#include "h4/graphics/canvas.hpp"
#include "h4/graphics/text.hpp"

namespace h4::ui {

TextButton::TextButton(core::Rect rect, std::string text, const formats::Font& font, Look look)
: Widget{rect}, text_{std::move(text)}, font_{&font}, look_{look} {
}

void TextButton::draw(const graphics::Canvas& canvas) const {
    const core::Rect local{{}, rect().size()};
    const auto state = visual();
    canvas.fill_rect(local,
                     state == PressVisual::pressed         ? look_.pressed :
                         state == PressVisual::highlighted ? look_.highlighted :
                                                             look_.released);
    canvas.stroke_rect(local, look_.edge);
    canvas.draw_text_block(
        *font_, text_, local, {look_.ink, std::nullopt}, {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre}
    );
}

}
