#include "h4/screens/adventure/creature_portrait.hpp"

#include <algorithm>
#include <format>
#include <string>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/formats/actor_sequence.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/combat_actor.hpp"
#include "h4/formats/sub_image.hpp"
#include "h4/graphics/canvas.hpp"

namespace h4::screens {

namespace {

const formats::CombatActor* model_of(const assets::Assets& assets, std::string_view keyword) {
    if (const formats::CombatActor* model = assets.combat_actor(keyword))
        return model;
    return assets.combat_actor("mage");
}

constexpr std::size_t kSouthWest = 4;

[[nodiscard]] double scale_to(core::Size box, const formats::BitmapGroup& layout) {
    const formats::Layer* whole = layout.find("1_of_1");
    if (whole == nullptr)
        return 1.0;
    const core::Rect w = whole->bounds();
    return std::min({static_cast<double>(box.width) / w.width(), static_cast<double>(box.height) / w.height(), 1.0});
}

[[nodiscard]] std::size_t copies_held(const formats::BitmapGroup& layout, core::Rect extent) {
    std::size_t copies = 3;
    for (; copies > 1; --copies) {
        const formats::Layer* first = layout.find(std::format("1_of_{}", copies));
        if (first != nullptr && extent.width() <= first->bounds().width() && extent.height() <= first->bounds().height())
            break;
    }
    return copies;
}

[[nodiscard]] core::Point scaled_centre(core::Rect rect, double scale) {
    return {static_cast<std::int32_t>((rect.left() + rect.right()) * scale / 2),
            static_cast<std::int32_t>((rect.top() + rect.bottom()) * scale / 2)};
}

}

CreaturePortrait::CreaturePortrait(core::Rect rect, const assets::Assets& assets, CreatureBoxModel box) : Widget{rect} {
    layout_ = assets.group("control.creature_model");
    background_ = assets.group(std::format("control.creature_model.{}", box.alignment));
    if (layout_ == nullptr)
        return;
    scale_                            = scale_to(rect.size(), *layout_);
    const formats::CombatActor* actor = model_of(assets, box.model);
    if (actor == nullptr)
        return;
    const CreatureIdle::Model idle = load_states(assets, *actor);
    sequence_                      = shown_sequence(assets, *actor);
    if (sequence_ == nullptr)
        return;
    const core::Rect extent = formats::frame_extent(sequence_->group(), sequence_->layers(), false);
    copies_                 = copies_held(*layout_, extent);
    place_copies(extent, idle);
}

CreatureIdle::Model CreaturePortrait::load_states(const assets::Assets& assets, const formats::CombatActor& actor) {
    CreatureIdle::Model idle;
    for (std::size_t s = 0; s < CreatureIdle::kStates; ++s) {
        const auto state             = static_cast<formats::CombatState>(s);
        const std::string_view named = actor.sequence(state, kSouthWest);
        states_[s]                   = named.empty() ? nullptr : assets.sequence(named);
        idle.frames[s]               = states_[s] != nullptr ? states_[s]->frames().size() : 0;
        idle.rates[s]                = actor.rate(state);
    }
    return idle;
}

const formats::ActorSequence* CreaturePortrait::shown_sequence(const assets::Assets& assets, const formats::CombatActor& actor) {
    std::string_view name = actor.sequence(formats::CombatState::walk, kSouthWest);
    if (name.empty() || assets.sequence(name) == nullptr)
        name = actor.sequence(formats::CombatState::wait, kSouthWest);
    return assets.sequence(name);
}

void CreaturePortrait::place_copies(core::Rect extent, const CreatureIdle::Model& idle) {
    const core::Point middle = scaled_centre(extent, scale_);
    for (std::size_t i = 0; i < copies_; ++i) {
        core::Rect guide;
        if (const formats::Layer* found = layout_->find(std::format("{}_of_{}", i + 1, copies_)))
            guide = found->bounds();
        anchors_[i] = scaled_centre(guide, scale_) - middle;
        copies_idle_[i].emplace(idle);
    }
}

void CreaturePortrait::advance(core::Instant now) {
    for (std::size_t i = 0; i < copies_; ++i)
        copies_idle_[i]->advance(now, seed_);
}

void CreaturePortrait::draw(const graphics::Canvas& parent) const {
    const core::Rect box = rect();
    const graphics::Canvas canvas = parent.child(core::Rect{{}, box.size()});
    if (background_ != nullptr) {
        const core::Rect art = background_->bounds();
        const core::Point at{static_cast<std::int32_t>((box.width() - art.width() * scale_) / 2),
                             static_cast<std::int32_t>((box.height() - art.height() * scale_) / 2)};
        for (const formats::Layer& layer : background_->layers())
            canvas.draw_layer_scaled(layer, at, scale_);
    }
    for (std::size_t i = 0; i < copies_; ++i) {
        const CreatureIdle& idle            = *copies_idle_[i];
        const formats::ActorSequence* shown = states_[std::to_underlying(idle.state())];
        if (shown == nullptr)
            continue;
        const auto layers       = shown->group().layers();
        const std::size_t frame = idle.frame();
        const core::Point at    = anchors_[i];
        const auto paint        = [&](std::span<const formats::FrameGroup> groups, std::int32_t base) {
            if (base >= 0)
                canvas.draw_layer_scaled(layers[static_cast<std::size_t>(base)], at, scale_);
            if (frame < groups.size())
                for (const std::uint32_t layer : groups[frame].layers)
                    canvas.draw_layer_scaled(layers[layer], at, scale_);
        };
        paint(shown->shadows(), shown->base_shadow());
        paint(shown->frames(), shown->base_frame());
    }
}

}
