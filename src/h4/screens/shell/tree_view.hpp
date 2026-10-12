#pragma once

#include "h4/core/geometry.hpp"
#include "h4/screens/shell/popup.hpp"
#include "h4/ui/widget_tree.hpp"

namespace h4::screens {

template <class Interface> class WidgetView : public Interface {
public:
    void handle(const platform::InputEvent& event) final {
        tree_.handle(event);
    }
    void paint(const graphics::Canvas& canvas) const final {
        tree_.paint(canvas);
    }
    [[nodiscard]] ui::HelpAt help_at(core::Point point) const final {
        return tree_.root().help_at(point);
    }
    [[nodiscard]] cursor::CursorState cursor_state() const final {
        return tree_.pointer();
    }

    [[nodiscard]] core::Size frame_size() const noexcept {
        return tree_.root().rect().size();
    }
    [[nodiscard]] const ui::Widget& root() const noexcept {
        return tree_.root();
    }

protected:
    explicit WidgetView(core::Size frame) : tree_{frame} {
    }

    [[nodiscard]] const ui::WidgetTree& tree() const noexcept {
        return tree_;
    }
    [[nodiscard]] ui::WidgetTree& tree() noexcept {
        return tree_;
    }

private:
    ui::WidgetTree tree_;
};

class ModalPopup : public WidgetView<Popup> {
public:
    [[nodiscard]] bool holds(core::Point ) const override {
        return true;
    }
    [[nodiscard]] bool covers(core::Point ) const override {
        return true;
    }

    [[nodiscard]] core::Rect dialog_rect() const noexcept {
        return dialog_ != nullptr ? dialog_->rect_in_root() : core::Rect{};
    }

protected:
    using WidgetView::WidgetView;

    void hold_dialog(ui::Widget& dialog) noexcept {
        dialog_ = &dialog;
    }
    [[nodiscard]] ui::Widget& dialog() const noexcept {
        return *dialog_;
    }

private:
    ui::Widget* dialog_ = nullptr;
};

}
