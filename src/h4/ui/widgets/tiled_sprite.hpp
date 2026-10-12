#pragma once

#include <cstdint>

#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

enum class TileRun : std::uint8_t { down, across };

class TiledSprite final : public Widget {
public:
    TiledSprite(const formats::Layer& tile, core::Rect run, core::Point group_origin, TileRun direction = TileRun::down) noexcept
    : Widget{run}, tile_{&tile}, group_origin_{group_origin}, direction_{direction} {
    }

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    const formats::Layer* tile_;
    core::Point group_origin_;
    TileRun direction_;
};

}
