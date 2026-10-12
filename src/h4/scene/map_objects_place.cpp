#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/core/ascii.hpp"
#include "h4/formats/actor_sequence.hpp"
#include "h4/formats/adv_actor.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/sub_image.hpp"
#include "h4/model/creature_actor.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/map.hpp"
#include "h4/model/roller.hpp"
#include "h4/scene/map_lattice.hpp"
#include "h4/scene/map_objects.hpp"
#include "h4/scene/map_objects_detail.hpp"

namespace h4::scene {

namespace {

template <class Height>
[[nodiscard]] std::int32_t vertex_average(const formats::ScenarioGrid& grid,
                                          core::Point origin,
                                          std::int32_t w,
                                          std::int32_t h,
                                          std::span<const std::uint8_t> marked,
                                          Height&& height) {
    std::int32_t sum = 0, count = 0;
    for (std::int32_t j = 0; j <= h; ++j)
        for (std::int32_t i = 0; i <= w; ++i) {
            if (marked[static_cast<std::size_t>(i + j * (w + 1))] == 0)
                continue;
            const std::int32_t vx = origin.x + i;
            const std::int32_t vy = origin.y + j;
            if (vx < 0 || vx >= grid.width())
                continue;
            const formats::GridSpan& span = grid.columns()[static_cast<std::size_t>(vx)];
            if (vy < span.start || vy >= span.stop)
                continue;
            sum += height(vx, vy);
            ++count;
        }
    return count == 0 ? 0 : (count / 2 + sum) / count;
}

[[nodiscard]] auto ground(const formats::ScenarioGrid& grid, std::int32_t level) {
    return [&grid, level](std::int32_t x, std::int32_t y) { return grid.elevation(x, y, level); };
}

constexpr std::array<std::uint8_t, 4> kArmyCorners{1, 1, 1, 1};

[[nodiscard]] std::int32_t
footprint_height(const formats::ScenarioGrid& grid, const formats::AdvObject& model, core::Point origin, std::int32_t level) {
    const std::int32_t w = model.size().width;
    const std::int32_t h = model.size().height;

    std::vector<std::uint8_t> marked(static_cast<std::size_t>((w + 1) * (h + 1)), 0);
    if (model.mode() == 0)
        std::ranges::fill(marked, std::uint8_t{1});
    else
        for (std::int32_t j = 0; j < h; ++j)
            for (std::int32_t i = 0; i < w; ++i) {
                if (!model.covers(i, j))
                    continue;
                for (std::int32_t dy = 0; dy <= 1; ++dy)
                    for (std::int32_t dx = 0; dx <= 1; ++dx)
                        marked[static_cast<std::size_t>((i + dx) + (j + dy) * (w + 1))] = 1;
            }
    return vertex_average(grid, origin, w, h, marked, ground(grid, level));
}

[[nodiscard]] int facing(model::Roller& facings) noexcept {
    return std::to_underlying(facings.next(2) == 0 ? formats::ActorDirection::se : formats::ActorDirection::sw);
}

constexpr std::uint32_t kModelFrameMs = 100;

[[nodiscard]] core::Point overlay_anchor(const formats::Animation& animation) {
    const core::Rect bounds = animation.group().bounds();
    return {animation.anchored_x() ? static_cast<std::int32_t>(animation.anchor_x()) : bounds.centre().x,
            animation.anchored_y() ? static_cast<std::int32_t>(animation.anchor_y()) : bounds.centre().y};
}

}

namespace detail {

std::int32_t army_height(const formats::ScenarioGrid& grid, core::Point cell, std::int32_t level) {
    return vertex_average(grid, cell, 1, 1, kArmyCorners, ground(grid, level));
}

}

using detail::kFlagAnimations;

void MapObjects::grow_to_overlays(Placed& placed) {
    for (const Overlay& overlay : {placed.ring, placed.flag})
        if (overlay.animation != nullptr)
            placed.rect = placed.rect.united(overlay.animation->group().bounds().translated(overlay.at));
}

MapObjects::Overlay
MapObjects::step_of(const formats::Animation& animation, core::Point pin, core::Point position, std::uint32_t phase) {
    if (animation.schedule().empty())
        return {};
    return {&animation, pin + position - overlay_anchor(animation), phase};
}

MapObjects::Placed
MapObjects::place_model(const formats::AdvObject& model, core::Point cell, std::int32_t level, std::uint32_t order, Standing standing) const {
    const formats::ScenarioGrid& grid = map_->terrain();

    Placed placed;
    placed.model  = &model;
    placed.group  = &model.group();
    placed.layers = &model.layers();
    placed.order  = order;
    placed.level  = level;
    placed.depth    = static_cast<std::int32_t>(std::int64_t{cell.x} + cell.y);
    placed.cell     = cell;
    placed.box      = model.size();
    placed.underlay = !model.overlay();
    std::int32_t height = 0;
    switch (standing) {
    case Standing::footprint: height = footprint_height(grid, model, cell, level); break;
    case Standing::corners: height = detail::army_height(grid, cell, level); break;
    case Standing::deck: height = deck_height(cell, model.size(), level); break;
    }
    placed.pin = box_pin(cell, grid.width(), height);
    placed.art = model.anchor() + placed.pin;
    placed.rect = formats::frame_extent(*placed.group, *placed.layers, true).translated(placed.art);
    placed.frame_ms = kModelFrameMs;
    placed.phase    = order;
    return placed;
}

void MapObjects::remove(std::uint32_t actor) noexcept {
    for (Placed& placed : placed_)
        if (placed.order == actor && placed.model != nullptr)
            placed.gone = true;
}

std::optional<core::Point> MapObjects::pin_of(const model::MapActor& actor) const noexcept {
    const std::span<const model::MapActor> actors = map_->actors();
    if (actors.empty() || &actor < actors.data() || &actor >= actors.data() + actors.size())
        return std::nullopt;
    const auto order = static_cast<std::uint32_t>(&actor - actors.data());
    for (const Placed& placed : placed_)
        if (placed.order == order)
            return placed.pin;
    return std::nullopt;
}

std::optional<core::Point> MapObjects::path_pin(std::size_t index) const noexcept {
    if (index >= path_.size())
        return std::nullopt;
    return path_[index].pin;
}

std::optional<core::Point> MapObjects::marker_pin() const noexcept {
    if (!marker_.has_value())
        return std::nullopt;
    return marker_->pin;
}

namespace {

[[nodiscard]] std::int64_t deck_key(std::int32_t x, std::int32_t y, std::int32_t level) noexcept {
    return (std::int64_t{level} << 40) | (std::int64_t{x} << 20) | std::int64_t{y & 0xFFFFF};
}

}

std::int32_t MapObjects::army_height(core::Point cell, std::int32_t level, bool bridge) const {
    const formats::ScenarioGrid& grid = map_->terrain();
    if (!bridge || decks_.empty())
        return detail::army_height(grid, cell, level);
    return deck_height(cell, {1, 1}, level);
}

std::int32_t MapObjects::deck_height(core::Point cell, core::Size box, std::int32_t level) const {
    const formats::ScenarioGrid& grid = map_->terrain();
    const std::vector<std::uint8_t> every(static_cast<std::size_t>((box.width + 1) * (box.height + 1)), 1);
    return vertex_average(grid, cell, box.width, box.height, every, [&](std::int32_t x, std::int32_t y) {
        const auto deck = decks_.find(deck_key(x, y, level));
        return deck != decks_.end() ? deck->second : grid.elevation(x, y, level);
    });
}

MapObjects::MapObjects(const assets::Assets& assets, const model::Map& map) : assets_{&assets}, map_{&map} {
    model::Roller facings{map.terrain_seed()};
    figures_.resize(map.actors().size());

    std::uint32_t order = 0;
    for (const model::MapActor& actor : map.actors()) {
        const std::uint32_t here = order++;

        if (actor.is_army()) {
            place_army(here, facing(facings));
            continue;
        }
        if (!model::shows_on_map(actor))
            continue;
        const formats::AdvObject* model = assets.object(actor.model);
        if (model == nullptr)
            continue;

        if (model->is_bridge()) {
            const std::int32_t base = footprint_height(map.terrain(), *model, actor.position, actor.level);
            for (std::int32_t i = 0; i <= model->size().width; ++i)
                for (std::int32_t j = 0; j <= model->size().height; ++j) {
                    const std::int32_t raise = model->deck(i, j);
                    if (raise == 0)
                        continue;
                    const std::int32_t vx = actor.position.x + i, vy = actor.position.y + j;
                    const std::int64_t key    = deck_key(vx, vy, actor.level);
                    const std::int32_t ground = decks_.contains(key) ? decks_[key] :
                                                                       map.terrain().elevation(vx, vy, actor.level);
                    decks_[key]               = std::max(ground, base + raise);
                }
        }
        Placed placed = place_model(*model, actor.position, actor.level, here);
        if (actor.owner < kFlagAnimations.size() - 1)
            if (const std::optional<core::Point> point = model->flag_point(); point.has_value())
                if (const formats::Animation* flag = assets.animation(kFlagAnimations[actor.owner]); flag != nullptr)
                    placed.flag = step_of(*flag, placed.pin, *point, here);
        grow_to_overlays(placed);
        placed_.push_back(placed);
    }
}

}
