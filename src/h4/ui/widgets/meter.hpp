#pragma once

#include <cstdint>
#include <vector>

#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/icon_cell.hpp"

namespace h4::ui {

class Meter final : public Widget {
public:
    struct Art {
        std::vector<const formats::Layer*> fills;
        std::int32_t extent           = 0;
        IconCell::Placement placement = IconCell::Placement::at_origin;
    };

    enum class Direction : std::uint8_t { rightwards, upwards };

    Meter(core::Rect full, Direction direction, Art art);

    void set_fraction(int value, int scale);

    [[nodiscard]] std::int32_t length() const noexcept {
        return length_;
    }

private:
    void draw(const graphics::Canvas& canvas) const override;

    Art art_;
    Direction direction_;
    std::int32_t extent_;
    std::int32_t length_ = 0;
};

}
