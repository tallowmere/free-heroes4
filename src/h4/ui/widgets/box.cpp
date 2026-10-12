#include "h4/ui/widgets/box.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

Box::Box(core::Rect rect, graphics::Rgba fill, std::optional<graphics::Rgba> edge) noexcept
: Widget{rect}, fill_{fill}, edge_{edge} {
}

void Box::draw(const graphics::Canvas& canvas) const {
    const core::Rect local{{}, rect().size()};
    canvas.fill_rect(local, fill_);
    if (edge_.has_value())
        canvas.stroke_rect(local, *edge_);
}

}
