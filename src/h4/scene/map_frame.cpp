#include "h4/scene/map_frame.hpp"

#include <algorithm>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/canvas.hpp"

namespace h4::scene {

namespace {

[[nodiscard]] std::int32_t tile_start(std::int32_t value, std::int32_t pitch) noexcept {
    const std::int32_t floor = value >= 0 ? value / pitch : -((pitch - 1 - value) / pitch);
    return floor * pitch;
}

void tile_art(const graphics::Canvas& canvas, const formats::Layer& art, const core::Rect& rect, core::Point at) {
    const core::Rect bounds = art.bounds();
    if (rect.empty() || bounds.empty())
        return;
    const graphics::Canvas clipped = canvas.clipped_to(rect);
    for (std::int32_t y = tile_start(rect.top() - at.y, bounds.height()); y < rect.bottom() - at.y; y += bounds.height())
        for (std::int32_t x = tile_start(rect.left() - at.x, bounds.width()); x < rect.right() - at.x; x += bounds.width())
            clipped.draw_layer(art, at + core::Point{x, y} - bounds.origin());
}

}

MapFrame::MapFrame(const assets::Assets& assets) {
    const formats::BitmapGroup* group = assets.group(kFrameGroup);
    if (group == nullptr)
        return;
    top_    = group->find("top");
    bottom_ = group->find("bottom");
    left_   = group->find("left");
    right_  = group->find("right");
    corner_ = group->find("corner");
}

core::Rect MapFrame::paint(const graphics::Canvas& canvas, const core::Rect& box) const {
    const core::Rect rect = canvas.local_clip();
    if (rect.empty())
        return rect;

    const core::Rect bands[]{
        core::Rect::from_edges(rect.left(), rect.top(), rect.right(), std::min(rect.bottom(), box.top())),
        core::Rect::from_edges(rect.left(), std::max(rect.top(), box.bottom()), rect.right(), rect.bottom()),
        core::Rect::from_edges(
            rect.left(), std::max(rect.top(), box.top()), std::min(rect.right(), box.left()), std::min(rect.bottom(), box.bottom())
        ),
        core::Rect::from_edges(
            std::max(rect.left(), box.right()), std::max(rect.top(), box.top()), rect.right(), std::min(rect.bottom(), box.bottom())
        ),
    };
    for (const core::Rect& band : bands)
        if (!band.empty())
            canvas.fill_rect(band, graphics::colors::black);

    if (top_ == nullptr || bottom_ == nullptr || left_ == nullptr || right_ == nullptr || corner_ == nullptr)
        return rect.intersected(box);

    const std::int32_t deep = top_->bounds().height();
    const std::int32_t wide = left_->bounds().width();

    tile_art(canvas,
             *top_,
             core::Rect::from_edges(box.left(), box.top() - deep, box.right(), box.top()).intersected(rect),
             {box.left(), box.top() - deep});
    tile_art(canvas,
             *bottom_,
             core::Rect::from_edges(box.left(), box.bottom(), box.right(), box.bottom() + deep).intersected(rect),
             {box.left(), box.bottom()});
    tile_art(canvas,
             *left_,
             core::Rect::from_edges(box.left() - wide, box.top(), box.left(), box.bottom()).intersected(rect),
             {box.left() - wide, box.top()});
    tile_art(canvas,
             *right_,
             core::Rect::from_edges(box.right(), box.top(), box.right() + wide, box.bottom()).intersected(rect),
             {box.right(), box.top()});

    const core::Size corner = corner_->bounds().size();
    for (const core::Point at : {core::Point{box.left() - corner.width, box.top() - corner.height},
                                 core::Point{box.right(), box.top() - corner.height},
                                 core::Point{box.left() - corner.width, box.bottom()},
                                 core::Point{box.right(), box.bottom()}})
        tile_art(canvas, *corner_, core::Rect{at, corner}.intersected(rect), at);

    return rect.intersected(box);
}

}
