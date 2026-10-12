#pragma once

#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class ClickArea final : public Widget {
public:
    explicit ClickArea(core::Rect rect) noexcept : Widget{rect} {
    }

    core::Signal<> pressed;
    core::Signal<> double_clicked;
    core::Signal<> clicked;

protected:
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_press(core::Point ) override {
        pressed.emit();
    }
    void on_double_click(core::Point ) override {
        double_clicked.emit();
    }
    void on_click() override {
        clicked.emit();
    }
};

}
