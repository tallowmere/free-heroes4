#pragma once

#include <memory>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/platform/input.hpp"
#include "h4/ui/widget.hpp"

namespace h4::graphics {
class Canvas;
}

namespace h4::ui {

class WidgetTree final {
public:
    explicit WidgetTree(core::Size size);

    [[nodiscard]] Panel& root() noexcept {
        return *root_;
    }
    [[nodiscard]] const Panel& root() const noexcept {
        return *root_;
    }

    void handle(const platform::InputEvent& event);
    void paint(const graphics::Canvas& canvas) const;

    [[nodiscard]] const Widget* hovered() const noexcept {
        return hovered_.get();
    }
    [[nodiscard]] cursor::CursorState pointer() const {
        return root_->pointer_at(at_);
    }

private:
    class WidgetRef {
    public:
        WidgetRef() = default;
        explicit WidgetRef(Widget* w) : widget_{w}, token_{w != nullptr ? w->lifetime_ : nullptr} {
        }
        [[nodiscard]] Widget* get() const noexcept {
            return token_.expired() ? nullptr : widget_;
        }

    private:
        Widget* widget_ = nullptr;
        std::weak_ptr<const char> token_;
    };

    void dispatch(const platform::InputEvent& event);
    void pointer_moved(core::Point p);
    void pointer_pressed(core::Point p, bool double_click);
    void pointer_released(core::Point p);

    std::unique_ptr<Panel> root_;
    std::vector<std::unique_ptr<Widget>> retired_;
    core::Point at_{-1, -1};
    WidgetRef hovered_;
    WidgetRef pressed_;
};

}
