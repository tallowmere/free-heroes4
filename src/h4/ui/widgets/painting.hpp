#pragma once

#include "h4/graphics/surface565.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class Painting final : public Widget {
public:
    Painting(const graphics::Surface565& surface, core::Point at) noexcept
    : Widget{core::Rect{at, surface.size()}}, surface_{&surface} {
    }
    explicit Painting(core::Rect rect) noexcept : Widget{rect} {
    }

    void set_surface(const graphics::Surface565* surface) noexcept {
        surface_ = surface;
    }

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    const graphics::Surface565* surface_ = nullptr;
};

}
