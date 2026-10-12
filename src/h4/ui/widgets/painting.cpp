#include "h4/ui/widgets/painting.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

void Painting::draw(const graphics::Canvas& canvas) const {
    if (surface_ != nullptr)
        canvas.draw_surface(*surface_, {});
}

}
