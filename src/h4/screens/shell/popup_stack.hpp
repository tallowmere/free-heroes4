#pragma once

#include <memory>

#include "h4/screens/shell/popup.hpp"
#include "h4/screens/shell/popup_host.hpp"

namespace h4::screens {

class PopupStack final : public Popup {
public:
    explicit PopupStack(std::unique_ptr<Popup> base) noexcept;

    [[nodiscard]] PopupHost& host() noexcept {
        return host_;
    }

    void handle(const platform::InputEvent& event) override {
        host_.handle(event);
    }
    void paint(const graphics::Canvas& canvas) const override {
        host_.paint(canvas);
    }
    void advance(core::Instant now) override {
        host_.advance(now);
    }
    [[nodiscard]] cursor::CursorState cursor_state() const override {
        return host_.cursor_state();
    }
    [[nodiscard]] ui::HelpAt help_at(core::Point point) const override {
        return host_.help_at(point);
    }
    [[nodiscard]] bool holds(core::Point point) const override {
        return base_->holds(point);
    }
    [[nodiscard]] bool covers(core::Point point) const override {
        return base_->covers(point);
    }

private:
    std::unique_ptr<Popup> base_;
    PopupHost host_;
};

}
