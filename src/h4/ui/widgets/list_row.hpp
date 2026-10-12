#pragma once

#include <cstdint>

#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/blend_window.hpp"

namespace h4::ui {

class BlendWindow;

class ListRow final : public Widget {
public:
    ListRow(core::Rect rect, core::Rect wash_rect, Wash wash);

    core::Signal<> clicked;
    core::Signal<> double_clicked;

    [[nodiscard]] Panel& plain() noexcept {
        return *plain_;
    }
    [[nodiscard]] Panel& selected() noexcept {
        return *hot_;
    }
    [[nodiscard]] Panel& overlay() noexcept {
        return *overlay_;
    }

    void clear();

    void set_selected(bool selected);
    [[nodiscard]] bool is_selected() const noexcept {
        return selected_;
    }

protected:
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_double_click(core::Point local) override;
    void on_click() override {
        clicked.emit();
    }

private:
    Panel* plain_      = nullptr;
    BlendWindow* wash_ = nullptr;
    Panel* hot_        = nullptr;
    Panel* overlay_    = nullptr;
    bool selected_     = false;
};

}
