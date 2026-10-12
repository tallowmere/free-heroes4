#include "h4/ui/widgets/icon_cell.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

namespace {

core::Point placement_offset(IconCell::Placement placement, const formats::Layer* layer) {
    if (layer == nullptr)
        return {};
    return placement == IconCell::Placement::at_origin ? core::Point{} - layer->bounds().origin() : core::Point{};
}

}

IconCell::IconCell(core::Rect cell, const formats::Layer* base, const formats::Layer* overlay, Placement placement)
: Widget{cell}, base_{base}, overlay_{overlay}, base_offset_{placement_offset(placement, base)} {
}

void IconCell::set_layer(const formats::Layer* base, Placement placement) {
    base_        = base;
    base_offset_ = placement_offset(placement, base);
}

void IconCell::set_overlay(const formats::Layer* pips) {
    overlay_ = pips;
}

void IconCell::draw(const graphics::Canvas& canvas) const {
    const graphics::Canvas inner = canvas.child(core::Rect{{}, rect().size()});
    if (base_ != nullptr)
        inner.draw_layer(*base_, base_offset_);
    if (overlay_ != nullptr)
        inner.draw_layer(*overlay_, {});
}

}
