#include "h4/ui/widgets/label.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

void Label::draw(const graphics::Canvas& canvas) const {
    canvas.draw_text_lines(
        *font_, wrapped_.lines(*font_, text_, rect().width()), core::Rect{{}, rect().size()}, style_.ink, style_.alignment
    );
}

}
