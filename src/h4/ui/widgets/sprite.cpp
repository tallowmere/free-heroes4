#include "h4/ui/widgets/sprite.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

void Sprite::draw(const graphics::Canvas& canvas) const {
    canvas.draw_layer(*layer_, core::Point{} - layer_->bounds().origin());
}

}
