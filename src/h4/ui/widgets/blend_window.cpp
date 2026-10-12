#include "h4/ui/widgets/blend_window.hpp"

#include <algorithm>

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

BlendWindow::BlendWindow(core::Rect rect, graphics::Rgba colour, std::int32_t strength) noexcept
: Widget{rect}, colour_{colour} {
    colour_.a = static_cast<std::uint8_t>(std::clamp(strength, 0, 15) * 255 / 15);
}

void BlendWindow::draw(const graphics::Canvas& canvas) const {
    canvas.fill_rect(core::Rect{{}, rect().size()}, colour_);
}

}
