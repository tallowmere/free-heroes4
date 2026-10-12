#pragma once

#include <string>

#include "h4/core/signal.hpp"
#include "h4/formats/font.hpp"
#include "h4/graphics/color.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class TextButton final : public Widget {
public:
    struct Look {
        graphics::Rgba released;
        graphics::Rgba highlighted;
        graphics::Rgba pressed;
        graphics::Rgba edge;
        graphics::Rgba ink;
    };

    TextButton(core::Rect rect, std::string text, const formats::Font& font, Look look);

    core::Signal<> clicked;

protected:
    void draw(const graphics::Canvas& canvas) const override;

    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_click() override {
        clicked.emit();
    }

private:
    std::string text_;
    const formats::Font* font_;
    Look look_;
};

}
