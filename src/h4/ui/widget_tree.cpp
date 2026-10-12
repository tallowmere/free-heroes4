#include "h4/ui/widget_tree.hpp"

#include <type_traits>
#include <variant>

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

WidgetTree::WidgetTree(core::Size size) : root_{std::make_unique<Panel>(core::Rect{{}, size})} {
}

void WidgetTree::handle(const platform::InputEvent& event) {
    root_->graveyard_ = &retired_;
    dispatch(event);
    root_->graveyard_ = nullptr;
    retired_.clear();
}

void WidgetTree::dispatch(const platform::InputEvent& event) {
    using namespace platform::events;
    if (const auto* moved = std::get_if<PointerMoved>(&event)) {
        pointer_moved(moved->position);
    } else if (const auto* pressed = std::get_if<PointerPressed>(&event)) {
        if (pressed->button == platform::PointerButton::primary)
            pointer_pressed(pressed->position, pressed->double_click);
    } else if (const auto* released = std::get_if<PointerReleased>(&event)) {
        if (released->button == platform::PointerButton::primary)
            pointer_released(released->position);
    }
}

void WidgetTree::pointer_moved(core::Point p) {
    at_ = p;
    if (Widget* const held = pressed_.get())
        held->on_drag(p - held->origin_in_root());
    Widget* const under  = root_->hit_test(p);
    Widget* const before = hovered_.get();
    if (under != before) {
        hovered_ = WidgetRef{under};
        if (before != nullptr)
            before->set_state(false, before->held_);
        if (Widget* now = hovered_.get())
            now->set_state(true, now->held_);
    }
    if (Widget* now = hovered_.get())
        now->on_pointer_moved(p - now->origin_in_root());
}

void WidgetTree::pointer_pressed(core::Point p, bool double_click) {
    pointer_moved(p);
    Widget* const target = hovered_.get();
    pressed_             = WidgetRef{target};
    if (target == nullptr)
        return;
    target->set_state(target->hovered_, true);
    target->on_press(p - target->origin_in_root());
    if (Widget* const still = pressed_.get(); double_click && still != nullptr)
        still->on_double_click(p - still->origin_in_root());
}

void WidgetTree::pointer_released(core::Point p) {
    Widget* const target = pressed_.get();
    pressed_             = WidgetRef{};
    if (target != nullptr) {
        const core::Point local = p - target->origin_in_root();
        const bool inside       = core::Rect{{}, target->rect().size()}.contains(local);
        const bool click = target->held_ && inside;
        target->set_state(target->hovered_, false);
        target->on_release(local);
        if (click)
            target->on_click();
    }
    pointer_moved(p);
}

void WidgetTree::paint(const graphics::Canvas& canvas) const {
    root_->paint(canvas);
}

}
