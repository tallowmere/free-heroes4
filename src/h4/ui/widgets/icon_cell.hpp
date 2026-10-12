#pragma once

#include <cstdint>

#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class IconCell final : public Widget {
public:
    enum class Placement : std::uint8_t { authored, at_origin };

    IconCell(core::Rect cell, const formats::Layer* base, const formats::Layer* overlay, Placement placement);

    void set_layer(const formats::Layer* base, Placement placement);
    void set_overlay(const formats::Layer* pips);

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    const formats::Layer* base_    = nullptr;
    const formats::Layer* overlay_ = nullptr;
    core::Point base_offset_{};
};

}
