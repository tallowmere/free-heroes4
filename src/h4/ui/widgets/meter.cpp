#include "h4/ui/widgets/meter.hpp"

#include <algorithm>

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

Meter::Meter(core::Rect full, Direction direction, Art art)
: Widget{full}, art_{std::move(art)}, direction_{direction},
  extent_{art_.extent != 0 ?
              art_.extent :
              (art_.fills.empty() ? 0 :
                                    (direction == Direction::rightwards ? art_.fills.front()->bounds().width() :
                                                                          art_.fills.front()->bounds().height()))} {
}

void Meter::set_fraction(int value, int scale) {
    if (scale < 1 || extent_ < 1) {
        length_ = 0;
        return;
    }
    length_ = std::clamp(extent_ * std::max(value, 0) / scale, 0, extent_);
}

void Meter::draw(const graphics::Canvas& canvas) const {
    for (const formats::Layer* layer : art_.fills) {
        if (layer == nullptr)
            continue;
        const core::Point offset = art_.placement == IconCell::Placement::at_origin ? core::Point{} - layer->bounds().origin() :
                                                                                      core::Point{};
        const core::Rect in_meter = layer->bounds().translated(offset);
        const core::Rect shown    = direction_ == Direction::rightwards ?
            core::Rect::from_edges(in_meter.left(), in_meter.top(), in_meter.left() + length_, in_meter.bottom()) :
            core::Rect::from_edges(in_meter.left(), in_meter.bottom() - length_, in_meter.right(), in_meter.bottom());
        canvas.clipped_to(shown).draw_layer(*layer, offset);
    }
}

}
