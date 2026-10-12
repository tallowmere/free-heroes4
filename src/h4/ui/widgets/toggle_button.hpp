#pragma once

#include <optional>

#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/button.hpp"

namespace h4::ui {

class ToggleButton final : public Widget {
public:
    ToggleButton(core::Point anchor, ButtonFaces faces, bool checked = false);

    core::Signal<bool> toggled;

    [[nodiscard]] bool checked() const noexcept {
        return checked_;
    }
    void set_checked(bool checked) noexcept {
        checked_ = checked;
    }

    void set_inset(ButtonFaces picture, core::Point at);

protected:
    void draw(const graphics::Canvas& canvas) const override;

    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_click() override;

private:
    struct Inset {
        ButtonFaces picture;
        core::Point at;
    };

    ButtonFaces faces_;
    core::Point origin_;
    std::optional<Inset> inset_;
    bool checked_ = false;
};

}
