#include <string>
#include <string_view>

#include "h4/assets/assets.hpp"
#include "h4/formats/actor_sequence.hpp"
#include "h4/formats/adv_actor.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/frame_rate.hpp"
#include "h4/formats/sub_image.hpp"
#include "h4/model/creature_actor.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/map.hpp"
#include "h4/scene/map_lattice.hpp"
#include "h4/scene/map_objects.hpp"
#include "h4/scene/map_objects_detail.hpp"

namespace h4::scene {

namespace {

using detail::kCellCentre;
using detail::kFlagAnimations;

[[nodiscard]] formats::ActorGroup actor_group(ArmyGroup group) noexcept {
    switch (group) {
    case ArmyGroup::prewalk: return formats::ActorGroup::prewalk;
    case ArmyGroup::walk: return formats::ActorGroup::walk;
    case ArmyGroup::postwalk: return formats::ActorGroup::postwalk;
    case ArmyGroup::wait: break;
    }
    return formats::ActorGroup::wait;
}

[[nodiscard]] const formats::AdvActor* figure_of(const assets::Assets& assets, const model::MapActor& actor) {
    std::string name;
    if (const formats::HeroRecord* leader = actor.army.leader(); leader != nullptr)
        name = model::hero_actor_name(*leader);
    else
        for (const formats::ArmySlot& slot : actor.army.slots) {
            if (slot.kind != formats::ArmySlot::Kind::creature)
                continue;
            name = std::string{model::creature_actor_keyword(slot.stack.creature)};
            break;
        }
    return name.empty() ? nullptr : assets.actor(name);
}

[[nodiscard]] const formats::ActorSequence*
sequence_of(const assets::Assets& assets, const formats::AdvActor& figure, ArmyGroup group, int facing) {
    const auto direction  = static_cast<formats::ActorDirection>(facing & 7);
    std::string_view cell = figure.sequence(actor_group(group), direction);
    if (cell.empty())
        cell = figure.sequence(formats::ActorGroup::wait, direction);
    return cell.empty() ? nullptr : assets.sequence(cell);
}

}

void MapObjects::place_army(std::uint32_t actor, int facing) {
    const model::MapActor& placed   = map_->actors()[actor];
    Figure& figure                  = figures_[actor];
    figure.actor                    = figure_of(*assets_, placed);
    figure.cell                     = placed.position;
    figure.level                    = placed.level;
    figure.pose                     = ArmyPose{ArmyGroup::wait, facing & 7, std::nullopt, {}};
    const std::optional<Placed> out = army_placement(actor, figure.cell, figure.level, figure.pose);
    if (!out.has_value())
        return;
    figure.placed = static_cast<std::int32_t>(placed_.size());
    placed_.push_back(*out);
}

std::optional<MapObjects::Placed>
MapObjects::army_placement(std::uint32_t actor, core::Point cell, std::int32_t level, const ArmyPose& pose) const {
    const model::MapActor& army     = map_->actors()[actor];
    const formats::AdvActor* figure = figures_[actor].actor;
    if (figure == nullptr)
        return std::nullopt;
    const formats::ActorSequence* sequence = sequence_of(*assets_, *figure, pose.group, pose.facing);
    if (sequence == nullptr)
        return std::nullopt;

    const core::Point pin   = box_pin(cell, map_->terrain().width(), army_height(cell, level, pose.bridge));
    const core::Point drawn = pin + pose.offset;

    Placed placed;
    placed.army_span = -figure->depth();
    placed.group     = &sequence->group();
    placed.layers    = &sequence->layers();
    placed.art       = drawn + kCellCentre + figure->anchor() + sequence->anchor();
    placed.pin       = drawn;
    placed.order     = actor;
    placed.level     = level;
    placed.depth     = static_cast<std::int32_t>(std::int64_t{cell.x} + cell.y);
    placed.cell      = cell;
    placed.box       = {1, 1};
    placed.rect      = formats::frame_extent(*placed.group, *placed.layers, true).translated(placed.art);
    if (pose.frame.has_value()) {
        placed.frame_ms = 0;
        placed.phase    = *pose.frame;
    } else {
        placed.frame_ms = formats::frame_ms_of(figure->frame_rate(actor_group(pose.group)));
        placed.phase    = actor;
    }

    if (!army.owned()) {
        if (const formats::Animation* ring = assets_->animation("highlight_ring"); ring != nullptr)
            placed.ring = step_of(*ring, drawn, kCellCentre, actor);
    } else if (army.owner < kFlagAnimations.size() - 1) {
        if (const formats::Animation* flag = assets_->animation(kFlagAnimations[army.owner]); flag != nullptr)
            placed.flag = step_of(*flag, drawn, figure->flag_point(), actor);
    }
    grow_to_overlays(placed);
    return placed;
}

void MapObjects::pose_army(std::uint32_t actor, core::Point cell, std::int32_t level, const ArmyPose& pose) {
    if (actor >= figures_.size() || figures_[actor].placed < 0)
        return;
    Figure& figure                     = figures_[actor];
    const std::optional<Placed> placed = army_placement(actor, cell, level, pose);
    if (!placed.has_value())
        return;
    figure.cell                                      = cell;
    figure.level                                     = level;
    figure.pose                                      = pose;
    placed_[static_cast<std::size_t>(figure.placed)] = *placed;
    if (selected_ == actor)
        select(&map_->actors()[actor]);
}

std::optional<std::pair<core::Point, std::int32_t>> MapObjects::army_cell(std::uint32_t actor) const noexcept {
    if (actor >= figures_.size() || figures_[actor].placed < 0)
        return std::nullopt;
    return std::pair{figures_[actor].cell, figures_[actor].level};
}

std::optional<ArmyPose> MapObjects::army_pose(std::uint32_t actor) const noexcept {
    if (actor >= figures_.size() || figures_[actor].placed < 0)
        return std::nullopt;
    return figures_[actor].pose;
}

std::uint32_t MapObjects::army_frames(std::uint32_t actor, ArmyGroup group, int facing) const {
    if (actor >= figures_.size() || figures_[actor].actor == nullptr)
        return 0;
    const formats::ActorSequence* sequence = sequence_of(*assets_, *figures_[actor].actor, group, facing);
    return sequence == nullptr ? 0U : static_cast<std::uint32_t>(sequence->layers().frames.size());
}

std::uint32_t MapObjects::army_frame_ms(std::uint32_t actor, ArmyGroup group) const noexcept {
    if (actor >= figures_.size() || figures_[actor].actor == nullptr)
        return 0;
    return formats::frame_ms_of(figures_[actor].actor->frame_rate(actor_group(group)));
}

ArmyWalk MapObjects::army_walk(std::uint32_t actor) const noexcept {
    if (actor >= figures_.size() || figures_[actor].actor == nullptr)
        return {};
    const formats::AdvActor::WalkGeometry& walk = figures_[actor].actor->walk_geometry();
    return ArmyWalk{walk.prewalk, walk.walk, walk.postwalk};
}

void MapObjects::select(const model::MapActor* army) {
    marker_.reset();
    selected_.reset();
    if (army == nullptr)
        return;
    const formats::AdvObject* model = assets_->object(detail::kSelectionMarker);
    if (model == nullptr)
        return;
    core::Point cell   = army->position;
    std::int32_t level = army->level;
    bool bridge        = false;
    if (const auto index = map_->index_of(army)) {
        const std::uint32_t actor = *index;
        selected_                 = actor;
        if (actor < figures_.size() && figures_[actor].placed >= 0) {
            cell   = figures_[actor].cell;
            level  = figures_[actor].level;
            bridge = figures_[actor].pose.bridge;
        }
    }
    marker_ = place_model(*model, cell, level, detail::kSelectionOrder, bridge ? Standing::deck : Standing::footprint);
}

}
