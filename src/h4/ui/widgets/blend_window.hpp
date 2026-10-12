#pragma once

#include <cstdint>

#include "h4/graphics/color.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

struct Wash {
    graphics::Rgba colour;
    std::int32_t strength;
};

inline constexpr Wash kSelectionWash{{0, 0, 200, 255}, 8};

class BlendWindow final : public Widget {
public:
    BlendWindow(core::Rect rect, graphics::Rgba colour, std::int32_t strength) noexcept;

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    graphics::Rgba colour_;
};

}
