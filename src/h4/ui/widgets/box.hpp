#pragma once

#include <optional>

#include "h4/graphics/color.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class Box final : public Widget {
public:
    Box(core::Rect rect, graphics::Rgba fill, std::optional<graphics::Rgba> edge) noexcept;

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    graphics::Rgba fill_;
    std::optional<graphics::Rgba> edge_;
};

}
