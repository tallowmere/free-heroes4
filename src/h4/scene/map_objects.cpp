#include "h4/scene/map_objects.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "h4/assets/assets.hpp"
#include "h4/core/clock.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/sub_image.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/model/map.hpp"
#include "h4/model/shroud.hpp"
#include "h4/scene/map_lattice.hpp"

namespace h4::scene {

namespace {

[[nodiscard]] std::size_t cycle(std::uint32_t now, std::uint32_t frame_ms, std::uint32_t phase, std::size_t count) {
    return core::cycle(std::chrono::milliseconds{now}, std::chrono::milliseconds{frame_ms}, phase, count);
}

template <class Visit>
bool each_frame_layer(std::span<const formats::FrameGroup> groups, std::int32_t base, std::uint32_t number, Visit&& visit) {
    if (base >= 0 && visit(static_cast<std::uint32_t>(base)))
        return true;
    for (const formats::FrameGroup& g : groups)
        if (g.number == number)
            for (std::uint32_t layer : g.layers)
                if (visit(layer))
                    return true;
    return false;
}

void draw_layers(const graphics::Canvas& canvas,
                 const formats::BitmapGroup& group,
                 core::Point origin,
                 std::span<const formats::FrameGroup> groups,
                 std::int32_t base,
                 std::uint32_t number) {
    const std::span<const formats::Layer> layers = group.layers();
    (void)each_frame_layer(groups, base, number, [&](std::uint32_t layer) {
        canvas.draw_layer(layers[layer], origin);
        return false;
    });
}

[[nodiscard]] std::uint32_t
frame_number(const formats::SubImageLayers& layers, std::uint32_t frame_ms, std::uint32_t phase, std::uint32_t now) {
    if (layers.frames.empty())
        return 0;
    return layers.frames[cycle(now, frame_ms, phase, layers.frames.size())].number;
}

}

void MapObjects::sort_column(std::span<const Placed* const> list,
                             const core::Rect& band,
                             std::vector<std::pair<std::int32_t, const Placed*>>& order) {
    order.clear();
    for (const Placed* placed : list) {
        if (placed->rect.intersected(band).empty())
            continue;
        const std::int32_t relative = object_column(band.left() - placed->pin.x);
        const std::int32_t span     = placed->model == nullptr ? placed->army_span : placed->model->span_at(relative);
        order.emplace_back(placed->depth - span, placed);
    }
    std::ranges::stable_sort(order, [](const auto& a, const auto& b) {
        return a.first != b.first ? a.first < b.first : a.second->order < b.second->order;
    });
}

void MapObjects::draw_frames(const graphics::Canvas& canvas,
                             core::Point camera,
                             const core::Rect& clip,
                             std::span<const Placed* const> list,
                             std::uint32_t now) const {
    if (list.empty())
        return;

    const std::int32_t first = object_column(clip.left() + camera.x);
    const std::int32_t last  = object_column(clip.right() - 1 + camera.x) + 1;
    for (std::int32_t column = first; column < last; ++column) {
        const core::Rect in_map{{column * kHalfTile, clip.top() + camera.y}, {kHalfTile, clip.height()}};
        const core::Rect band = in_map.translated(core::Point{} - camera);

        auto& order = column_order_;
        sort_column(list, in_map, order);

        const graphics::Canvas clipped = canvas.clipped_to(band);
        const auto overlay = [&](const Overlay& piece) {
            if (const formats::Layer* layer = overlay_layer(piece, now))
                clipped.draw_layer(*layer, piece.at - camera);
        };
        for (const auto& [key, placed] : order) {
            overlay(placed->ring);
            draw_layers(clipped,
                        *placed->group,
                        placed->art - camera,
                        placed->layers->frames,
                        placed->layers->base_frame,
                        frame_number(*placed->layers, placed->frame_ms, placed->phase, now));
            overlay(placed->flag);
        }
    }
}

bool MapObjects::shown(const Placed& placed, std::int32_t level, const model::Shroud* seen) {
    if (seen == nullptr || placed.model != nullptr)
        return true;
    return in_sight(placed, level, *seen);
}

bool MapObjects::in_sight(const Placed& placed, std::int32_t level, const model::Shroud& seen) {
    for (std::int32_t y = 0; y < placed.box.height; ++y)
        for (std::int32_t x = 0; x < placed.box.width; ++x)
            if (seen.level(placed.cell.x + x, placed.cell.y + y, level) == model::kFogVisible)
                return true;
    return false;
}

void MapObjects::paint(const graphics::Canvas& canvas, core::Point camera, std::int32_t level, const model::Shroud* seen, std::uint32_t now) const {
    const core::Rect clip = canvas.local_clip();
    if (clip.empty())
        return;

    under_.clear();
    over_.clear();
    const auto consider = [&](const Placed& placed) {
        if (placed.gone || placed.level != level)
            return;
        if (placed.rect.translated(core::Point{} - camera).intersected(clip).empty())
            return;
        if (!shown(placed, level, seen))
            return;
        (placed.underlay ? under_ : over_).push_back(&placed);
    };
    for (const Placed& placed : placed_)
        consider(placed);
    for (const Placed& placed : path_)
        consider(placed);
    if (marker_.has_value())
        consider(*marker_);

    draw_frames(canvas, camera, clip, under_, now);
    for (const std::vector<const Placed*>* set : {&under_, &over_})
        for (const Placed* placed : *set)
            draw_layers(canvas,
                        *placed->group,
                        placed->art - camera,
                        placed->layers->shadows,
                        placed->layers->base_shadow,
                        frame_number(*placed->layers, placed->frame_ms, placed->phase, now));
    draw_frames(canvas, camera, clip, over_, now);
}

void MapObjects::show_path(std::span<const PathMark> marks) {
    path_.clear();
    auto order = static_cast<std::uint32_t>(map_->actors().size());
    for (const PathMark& mark : marks)
        if (const formats::AdvObject* model = assets_->object(path_arrow_name(mark)); model != nullptr)
            path_.push_back(place_model(*model, mark.tile, mark.level, order++, mark.bridge ? Standing::deck : Standing::corners));
}

const formats::Layer* MapObjects::overlay_layer(const Overlay& piece, std::uint32_t now) noexcept {
    if (piece.animation == nullptr)
        return nullptr;
    return piece.animation->layer_at(std::chrono::milliseconds{now}, piece.phase);
}

bool MapObjects::hit(const Placed& placed, core::Point point, std::uint32_t now) {
    const auto on      = [&](const formats::Layer& layer, core::Point origin) { return layer.hit(point - origin); };
    const auto overlay = [&](const Overlay& piece) {
        const formats::Layer* layer = overlay_layer(piece, now);
        return layer != nullptr && on(*layer, piece.at);
    };
    const std::span<const formats::Layer> layers = placed.group->layers();
    const std::uint32_t number                   = frame_number(*placed.layers, placed.frame_ms, placed.phase, now);
    if (each_frame_layer(placed.layers->frames, placed.layers->base_frame, number, [&](std::uint32_t layer) {
            return on(layers[layer], placed.art);
        }))
        return true;
    return overlay(placed.flag) || overlay(placed.ring);
}

bool MapObjects::drawn(const model::MapActor& actor, const model::Shroud* seen) const noexcept {
    const std::span<const model::MapActor> actors = map_->actors();
    if (actors.empty() || &actor < actors.data() || &actor >= actors.data() + actors.size())
        return false;
    const auto order = static_cast<std::uint32_t>(&actor - actors.data());
    for (const Placed& placed : placed_)
        if (placed.order == order && !placed.gone)
            return shown(placed, placed.level, seen);
    return false;
}

std::optional<std::uint32_t>
MapObjects::pick(core::Point point, std::int32_t level, const model::Shroud* seen, std::uint32_t now) const {
    const core::Rect band{{object_column(point.x) * kHalfTile, point.y}, {kHalfTile, 1}};
    under_.clear();
    over_.clear();
    for (const Placed& placed : placed_) {
        if (placed.gone || placed.level != level || !placed.rect.contains(point) || !shown(placed, level, seen))
            continue;
        (placed.underlay ? under_ : over_).push_back(&placed);
    }
    for (const std::vector<const Placed*>* set : {&over_, &under_}) {
        sort_column(*set, band, column_order_);
        for (auto it = column_order_.rbegin(); it != column_order_.rend(); ++it)
            if (hit(*it->second, point, now))
                return it->second->order;
    }
    return std::nullopt;
}

}
