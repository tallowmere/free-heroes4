#include "h4/screens/adventure/mini_map_window.hpp"

#include "h4/screens/adventure/map_window.hpp"

namespace h4::screens {

void MiniMapWindow::show_map(const assets::Assets& assets, const model::Map& map) {
    if (const scene::MapRenderer* renderer = map_->renderer())
        image_.emplace(assets, map, *renderer, rect().size());
}

core::Point MiniMapWindow::map_pixel(core::Point local) const noexcept {
    const core::Size panel             = rect().size();
    const scene::MapRenderer* renderer = map_->renderer();
    if (renderer == nullptr || panel.width <= 0 || panel.height <= 0)
        return {};
    return scene::MiniMap::to_box(local, panel, renderer->extent());
}

void MiniMapWindow::draw(const graphics::Canvas& canvas) const {
    const scene::MapRenderer* renderer = map_->renderer();
    if (!image_ || renderer == nullptr)
        return;
    image_->paint(canvas, {}, core::Rect{map_->camera(), map_->rect().size()}, renderer->extent());
}

}
