#include "h4/ui/widgets/tiled_sprite.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

void TiledSprite::draw(const graphics::Canvas& canvas) const {
    const core::Rect tile   = tile_->bounds();
    const bool across       = direction_ == TileRun::across;
    const std::int32_t step = across ? tile.width() : tile.height();
    if (tile_->is_placeholder() || step <= 0)
        return;
    const graphics::Canvas run = canvas.child(core::Rect{{}, rect().size()});
    const core::Point kept     = group_origin_ - rect().origin();
    const std::int32_t length  = across ? rect().width() : rect().height();
    for (std::int32_t at = 0; at < length; at += step)
        run.draw_layer(*tile_, across ? core::Point{at - tile.left(), kept.y} : core::Point{kept.x, at - tile.top()});
}

}
