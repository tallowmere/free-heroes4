#include "h4/ui/widgets/scrollbar.hpp"

#include <algorithm>

#include "h4/ui/widgets/sprite.hpp"
#include "h4/ui/widgets/tiled_sprite.hpp"

namespace h4::ui {

std::optional<ScrollbarArt> ScrollbarArt::from_group(const formats::BitmapGroup& group) {
    auto up                          = ButtonFaces::from_group(group, "up");
    auto down                        = ButtonFaces::from_group(group, "down");
    const formats::Layer* background = group.find("background");
    const formats::Layer* thumb      = group.find("thumb");
    if (!up || !down || background == nullptr || thumb == nullptr)
        return std::nullopt;
    return ScrollbarArt{*up, *down, background, thumb};
}

std::int32_t ScrollbarArt::width() const noexcept {
    return std::max({up.extent().right(), down.extent().right(), background->bounds().right()});
}

std::int32_t ScrollbarArt::height() const noexcept {
    return std::max({up.extent().bottom(), down.extent().bottom(), background->bounds().bottom()});
}

Scrollbar::Scrollbar(core::Point origin, std::int32_t length, const ScrollbarArt& art, std::int32_t min, std::int32_t max, ScrollbarOrientation orientation)
: Widget{{}}, thumb_layer_{art.thumb}, orientation_{orientation}, min_{min}, max_{std::max(min, max)} {
    const core::Rect up_extent     = art.up.extent();
    const core::Rect down_extent   = art.down.extent();
    const core::Rect bg            = art.background->bounds();
    const std::int32_t up_length   = extent(up_extent);
    const std::int32_t down_length = extent(down_extent);
    const std::int32_t run         = std::max(0, length - up_length - down_length);

    if (vertical()) {
        set_rect(core::Rect{origin, {art.width(), length}});
        track_ = core::Rect::from_edges(bg.left(), up_length, bg.right(), length - down_length);
    } else {
        set_rect(core::Rect{origin, {length, art.height()}});
        track_ = core::Rect::from_edges(up_length, bg.top(), length - down_length, bg.bottom());
    }
    page_ = (max_ - min_) / 5;

    auto& up = emplace_child<Button>(core::Point{}, art.up);
    if (vertical()) {
        auto& track = emplace_child<Panel>(core::Rect{{0, up_length}, {rect().width(), run}});
        track.emplace_child<TiledSprite>(*art.background, core::Rect{{}, track.rect().size()}, core::Point{0, -bg.top()});
    } else {
        auto& track = emplace_child<Panel>(core::Rect{{up_length, 0}, {run, rect().height()}});
        track.emplace_child<TiledSprite>(*art.background, core::Rect{{}, track.rect().size()}, core::Point{}, TileRun::across);
    }
    if (extent(art.thumb->bounds()) <= extent(track_))
        thumb_ = &emplace_child<Sprite>(*art.thumb);
    const core::Point down_anchor = vertical() ? core::Point{0, length - down_length - down_extent.top()} :
                                                 core::Point{length - down_length - down_extent.left(), 0};
    auto& down                    = emplace_child<Button>(down_anchor, art.down);

    connections_.on(up.clicked, [this] { set_position(position_ - 1); });
    connections_.on(down.clicked, [this] { set_position(position_ + 1); });
    position_ = min_;
    place_thumb();
}

std::int32_t Scrollbar::along(core::Point point) const noexcept {
    return vertical() ? point.y : point.x;
}

std::int32_t Scrollbar::start(const core::Rect& rect) const noexcept {
    return vertical() ? rect.top() : rect.left();
}

std::int32_t Scrollbar::extent(const core::Rect& rect) const noexcept {
    return vertical() ? rect.height() : rect.width();
}

void Scrollbar::set_range(std::int32_t min, std::int32_t max) {
    min_      = min;
    max_      = std::max(min, max);
    position_ = std::clamp(position_, min_, max_);
    place_thumb();
}

void Scrollbar::set_position(std::int32_t position) {
    const std::int32_t clamped = std::clamp(position, min_, max_);
    if (clamped == position_)
        return;
    position_ = clamped;
    place_thumb();
    changed.emit(position_);
}

std::int32_t Scrollbar::travel() const noexcept {
    return thumb_ != nullptr ? extent(track_) - extent(thumb_layer_->bounds()) : 0;
}

core::Rect Scrollbar::thumb_rect() const noexcept {
    return thumb_ != nullptr ? thumb_->rect() : core::Rect{};
}

void Scrollbar::place_thumb() {
    if (thumb_ == nullptr)
        return;
    const std::int32_t range = max_ - min_;
    std::int32_t at          = start(track_);
    if (range >= 1 && travel() >= 1)
        at += ((position_ - min_) * travel() + range / 2) / range;
    const core::Rect authored = thumb_layer_->bounds();
    thumb_->set_rect(vertical() ? core::Rect{{authored.left(), at}, authored.size()} :
                                  core::Rect{{at, authored.top()}, authored.size()});
}

void Scrollbar::on_press(core::Point local) {
    if (thumb_ == nullptr || !track_.contains(local))
        return;
    const core::Rect thumb = thumb_rect();
    const std::int32_t at  = along(local);
    if (at < start(thumb))
        set_position(position_ - page_);
    else if (at >= start(thumb) + extent(thumb))
        set_position(position_ + page_);
    else
        grab_ = at - start(thumb);
}

void Scrollbar::on_drag(core::Point local) {
    const std::int32_t span = travel();
    if (!grab_ || span < 1)
        return;
    const std::int32_t offset = std::clamp(along(local) - *grab_ - start(track_), 0, span);
    set_position(min_ + (offset * (max_ - min_) + span / 2) / span);
}

void Scrollbar::on_release(core::Point ) {
    grab_.reset();
}

}
