#include "h4/ui/widget.hpp"

#include <algorithm>

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

Widget::~Widget() = default;

void Widget::set_enabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled)
        set_state(false, false);
}

void Widget::set_state(bool hovered, bool held) {
    if (hovered == hovered_ && held == held_)
        return;
    hovered_ = hovered;
    held_    = held;
    on_state_changed();
}

PressVisual Widget::visual() const noexcept {
    if (held_ && hovered_)
        return PressVisual::pressed;
    if (hovered_)
        return PressVisual::highlighted;
    return PressVisual::released;
}

void Widget::cancel_press() {
    set_state(hovered_, false);
}

std::vector<std::unique_ptr<Widget>>* Widget::graveyard() noexcept {
    Widget* root = this;
    while (root->parent_ != nullptr)
        root = root->parent_;
    return root->graveyard_;
}

void Widget::drop_child(const Widget& child) {
    std::unique_ptr<Widget> owned = remove_child(child);
    if (auto* retired = graveyard(); owned != nullptr && retired != nullptr)
        retired->push_back(std::move(owned));
}

void Widget::clear_children() {
    if (auto* retired = graveyard()) {
        for (auto& child : children_) {
            child->parent_ = nullptr;
            retired->push_back(std::move(child));
        }
    }
    children_.clear();
}

void Widget::adopt(std::unique_ptr<Widget> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

std::unique_ptr<Widget> Widget::remove_child(const Widget& child) {
    const auto it = std::ranges::find_if(children_, [&child](const auto& owned) { return owned.get() == &child; });
    if (it == children_.end())
        return nullptr;
    std::unique_ptr<Widget> out = std::move(*it);
    children_.erase(it);
    out->parent_ = nullptr;
    return out;
}

void Widget::attach(std::unique_ptr<Widget> child) {
    adopt(std::move(child));
}

void Widget::place_before(const Widget& child, const Widget& sibling) {
    const auto is   = [](const Widget& w) { return [&w](const auto& owned) { return owned.get() == &w; }; };
    const auto from = std::ranges::find_if(children_, is(child));
    const auto to   = std::ranges::find_if(children_, is(sibling));
    if (from == children_.end() || to == children_.end() || from == to)
        return;
    if (from < to)
        std::rotate(from, from + 1, to);
    else
        std::rotate(to, from, from + 1);
}

bool Widget::shown() const noexcept {
    for (const Widget* w = this; w != nullptr; w = w->parent_)
        if (!w->visible_)
            return false;
    return true;
}

core::Point Widget::origin_in_root() const noexcept {
    core::Point origin{};
    for (const Widget* w = this; w != nullptr && w->parent_ != nullptr; w = w->parent_)
        origin = origin + w->rect_.origin();
    return origin;
}

core::Rect Widget::rect_in_root() const noexcept {
    return core::Rect{origin_in_root(), rect_.size()};
}

void Widget::paint(const graphics::Canvas& parent_canvas) const {
    if (!visible_)
        return;
    draw(parent_canvas.translated(rect_.origin()));
    const graphics::Canvas inner = parent_canvas.child(rect_);
    if (inner.local_clip().empty())
        return;
    for (const auto& child : children_)
        child->paint(inner);
}

const Widget* Widget::window_at(core::Point point_in_parent) const noexcept {
    if (!visible_ || !rect_.contains(point_in_parent))
        return nullptr;
    const core::Point local = point_in_parent - rect_.origin();
    if (!holds(local))
        return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (const Widget* window = (*it)->window_at(local))
            return window;
    return see_through_ ? nullptr : this;
}

Help Widget::help_here(core::Point local) const {
    if (const auto* source = std::get_if<HelpFunction>(&help_))
        return (*source)(local);
    return std::get<Help>(help_);
}

HelpAt Widget::help_at(core::Point point_in_parent) const {
    HelpAt out;
    bool right_click_found = false;
    const Widget* window   = window_at(point_in_parent);
    const core::Point in_root = point_in_parent + (parent_ != nullptr ? parent_->origin_in_root() : core::Point{});
    for (; window != nullptr; window = window->parent_) {
        const core::Point local = in_root - window->origin_in_root();
        const Help words        = window->help_here(local);
        if (out.balloon.empty())
            out.balloon = words.balloon;
        if (out.silence == Silence::none) {
            out.silence = window->silence_;
            out.silent  = window->silent_;
        }
        if (!right_click_found && (window->right_click_ || !words.right_click.empty())) {
            right_click_found     = true;
            out.right_click_owner = window->popup_at_pointer_ ? core::Rect{in_root, {}} : window->rect_in_root();
            if (window->right_click_)
                out.take_right_click = [action = window->right_click_, local] { action(local); };
            else
                out.right_click = words.right_click;
        }
        if (window == this)
            break;
    }
    return out;
}

cursor::CursorState Widget::pointer_at(core::Point point_in_parent) const {
    const core::Point in_root = point_in_parent + (parent_ != nullptr ? parent_->origin_in_root() : core::Point{});
    for (const Widget* window = window_at(point_in_parent); window != nullptr; window = window->parent_) {
        if (const auto* fixed = std::get_if<cursor::CursorState>(&window->pointer_))
            return *fixed;
        if (const auto* source = std::get_if<PointerFunction>(&window->pointer_))
            return (*source)(in_root - window->origin_in_root());
        if (window == this)
            break;
    }
    return {};
}

Widget* Widget::hit_test(core::Point point_in_parent) noexcept {
    if (!visible_ || !enabled_ || !rect_.contains(point_in_parent))
        return nullptr;
    const core::Point local = point_in_parent - rect_.origin();
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (Widget* hit = (*it)->hit_test(local))
            return hit;
    return accepts_pointer() ? this : nullptr;
}

core::Rect shown_rect(const Widget* widget) noexcept {
    return widget != nullptr && widget->shown() ? widget->rect_in_root() : core::Rect{};
}

}
