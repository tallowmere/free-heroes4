#include "h4/ui/widgets/toggle_button.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

ToggleButton::ToggleButton(core::Point anchor, ButtonFaces faces, bool checked)
: Widget{faces.placed(anchor)}, faces_{faces}, origin_{faces.draw_origin()}, checked_{checked} {
}

void ToggleButton::set_inset(ButtonFaces picture, core::Point at) {
    inset_                   = Inset{picture, at};
    const core::Point anchor = rect().origin() + origin_;
    const core::Rect fitted  = faces_.extent().translated(anchor);
    origin_                  = anchor - fitted.origin();
    set_rect(fitted);
}

void ToggleButton::draw(const graphics::Canvas& canvas) const {
    const bool pressed = checked_ || (held() && hovered());
    if (inset_) {
        const formats::Layer& picture = inset_->picture.toggle_face(pressed, hovered(), enabled());
        canvas.draw_layer(picture, origin_ + inset_->at - picture.bounds().origin());
    }
    canvas.draw_layer(faces_.toggle_face(pressed, hovered(), enabled()), origin_);
}

void ToggleButton::on_click() {
    checked_ = !checked_;
    toggled.emit(checked_);
}

}
