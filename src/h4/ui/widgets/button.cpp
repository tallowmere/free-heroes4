#include "h4/ui/widgets/button.hpp"

#include <string>

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

namespace {

const formats::Layer* face_layer(const formats::BitmapGroup& group, std::string_view base, std::string_view state) {
    std::string name{base};
    if (!name.empty())
        name += ButtonFaces::kJoin;
    name += state;
    const formats::Layer* layer = group.find(name);
    return layer != nullptr && !layer->is_placeholder() ? layer : nullptr;
}

}

std::optional<ButtonFaces> ButtonFaces::from_group(const formats::BitmapGroup& group, std::string_view base) {
    const formats::Layer* released = face_layer(group, base, kStateNames[0]);
    if (released == nullptr)
        return std::nullopt;
    ButtonFaces faces{*released,
                      face_layer(group, base, kStateNames[1]),
                      face_layer(group, base, kStateNames[2]),
                      face_layer(group, base, kStateNames[3]),
                      face_layer(group, base, kStateNames[4])};
    return faces;
}

const formats::Layer& ButtonFaces::toggle_face(bool pressed, bool hovered, bool enabled) const noexcept {
    if (!enabled && disabled_ != nullptr)
        return *disabled_;
    if (pressed && hovered && highlighted_pressed_ != nullptr)
        return *highlighted_pressed_;
    if (pressed && pressed_ != nullptr)
        return *pressed_;
    if (hovered && highlighted_ != nullptr)
        return *highlighted_;
    return *released_;
}

const formats::Layer& ButtonFaces::face(PressVisual visual, bool enabled) const noexcept {
    const formats::Layer* chosen = nullptr;
    if (!enabled)
        chosen = disabled_;
    else if (visual == PressVisual::pressed)
        chosen = pressed_;
    else if (visual == PressVisual::highlighted)
        chosen = highlighted_;
    return chosen != nullptr ? *chosen : *released_;
}

ButtonFaces ButtonFaces::with_disabled(const formats::Layer& face) const noexcept {
    ButtonFaces out = *this;
    out.disabled_   = &face;
    return out;
}

core::Rect ButtonFaces::extent() const noexcept {
    core::Rect out = released_->bounds();
    for (const formats::Layer* l : {pressed_, highlighted_, highlighted_pressed_, disabled_})
        if (l != nullptr)
            out = out.united(l->bounds());
    return out;
}

core::Rect ButtonFaces::placed(core::Point anchor) const noexcept {
    const core::Point corner = released_->bounds().origin();
    const core::Rect all     = extent();
    return core::Rect::from_edges(anchor.x + corner.x, anchor.y + corner.y, anchor.x + all.right(), anchor.y + all.bottom());
}

core::Point ButtonFaces::draw_origin() const noexcept {
    return core::Point{} - released_->bounds().origin();
}

Button::Button(core::Point anchor, ButtonFaces faces) : Widget{faces.placed(anchor)}, faces_{faces} {
}

core::Rect Button::art_bounds() const noexcept {
    return faces_.extent().translated(rect().origin() + faces_.draw_origin());
}

void Button::set_faces(ButtonFaces faces) {
    const core::Point anchor = rect().origin() - faces_.released().bounds().origin();
    faces_                   = faces;
    set_rect(faces_.placed(anchor));
}

void Button::draw(const graphics::Canvas& canvas) const {
    const formats::Layer& face = faces_.face(visual(), enabled());
    const core::Point faces_origin = faces_.draw_origin();
    canvas.draw_layer(face, faces_origin);
    if (!caption_ || caption_->font == nullptr)
        return;
    const bool pressed   = enabled() && visual() == PressVisual::pressed;
    const core::Rect box = !enabled() ? caption_->disabled : pressed ? caption_->pressed : caption_->released;
    canvas.draw_text_lines(*caption_->font,
                           wrapped_.lines(*caption_->font, caption_->text, box.width()),
                           box.translated(faces_origin),
                           enabled() ? caption_->ink : caption_->disabled_ink,
                           {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre});
}

}
