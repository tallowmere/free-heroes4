#include "h4/screens/shell/popup_host.hpp"

#include <variant>

namespace h4::screens {

namespace {
constexpr core::Point kNowhere{-1, -1};
}

void PopupHost::open(std::unique_ptr<Popup> popup) {
    if (dispatching_) {
        incoming_      = std::move(popup);
        close_pending_ = true;
        return;
    }
    popup_ = std::move(popup);
}

void PopupHost::close() noexcept {
    if (dispatching_) {
        close_pending_ = true;
        incoming_.reset();
        return;
    }
    popup_.reset();
}

void PopupHost::set_base(View& base) noexcept {
    base_ = &base;
    close();
}

bool PopupHost::shows(const Popup& popup) const noexcept {
    if (dispatching_ && close_pending_)
        return incoming_.get() == &popup;
    return popup_.get() == &popup;
}

bool PopupHost::has_popup() const noexcept {
    if (dispatching_ && close_pending_)
        return incoming_ != nullptr;
    return popup_ != nullptr;
}

void PopupHost::handle(const platform::InputEvent& event) {
    dispatching_ = true;
    dispatch(event);
    dispatching_ = false;
    settle();
}

void PopupHost::dispatch(const platform::InputEvent& event) {
    using namespace platform::events;
    if (popup_ == nullptr) {
        base_->handle(event);
        return;
    }
    if (const auto* moved = std::get_if<PointerMoved>(&event)) {
        popup_->handle(event);
        base_->handle(popup_->covers(moved->position) ? PointerMoved{kNowhere} : *moved);
    } else if (const auto* pressed = std::get_if<PointerPressed>(&event)) {
        if (popup_->holds(pressed->position)) {
            popup_->handle(event);
            return;
        }
        close();
        base_->handle(PointerMoved{pressed->position});
        base_->handle(event);
    } else if (const auto* released = std::get_if<PointerReleased>(&event)) {
        if (popup_->covers(released->position))
            popup_->handle(event);
        else
            base_->handle(event);
    } else {
        base_->handle(event);
    }
}

void PopupHost::settle() noexcept {
    if (!close_pending_)
        return;
    close_pending_ = false;
    popup_         = std::move(incoming_);
}

void PopupHost::advance(core::Instant now) {
    base_->advance(now);
    if (popup_ != nullptr)
        popup_->advance(now);
}

void PopupHost::paint(const graphics::Canvas& canvas) const {
    base_->paint(canvas);
    if (popup_ != nullptr)
        popup_->paint(canvas);
}

ui::HelpAt PopupHost::help_at(core::Point point) const {
    if (popup_ != nullptr && popup_->covers(point))
        return popup_->help_at(point);
    return base_->help_at(point);
}

cursor::CursorState PopupHost::cursor_state() const {
    return popup_ != nullptr ? popup_->cursor_state() : base_->cursor_state();
}

}
