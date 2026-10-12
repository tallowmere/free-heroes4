#include "h4/scene/viewer_shroud.hpp"

#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/model/map.hpp"

namespace h4::scene {

namespace {

[[nodiscard]] core::Size footprint_of(const assets::Assets& assets, const model::MapActor& actor) {
    if (actor.is_army())
        return {1, 1};
    const formats::AdvObject* art = assets.object(actor.model);
    return art == nullptr ? core::Size{1, 1} : art->size();
}

}

ViewerShroud::ViewerShroud(const assets::Assets& assets, const model::Map& map, std::optional<model::PlayerColour> viewer, std::uint8_t team)
: assets_{&assets}, map_{&map}, transitions_{assets.transitions()}, team_{model::Shroud::team_mask(viewer, team)} {
    for (const model::MapActor& actor : map.actors())
        at_.push_back({actor.position, actor.level});
    if (!viewer || transitions_ == nullptr)
        return;
    shroud_ = model::Shroud::at_start(
        map, viewer, team, [&assets](const model::MapActor& actor) { return footprint_of(assets, actor); }, *transitions_
    );
}

model::Eye ViewerShroud::eye_of(const model::MapActor& actor, const Cell& at) const {
    const core::Size box = footprint_of(*assets_, actor);
    return model::Eye{at.cell, box, at.level, model::sight_radius(actor, box)};
}

void ViewerShroud::moved(std::uint32_t actor, core::Point cell, std::int32_t level) {
    if (actor >= at_.size() || at_[actor] == Cell{cell, level})
        return;
    const Cell from   = std::exchange(at_[actor], Cell{cell, level});
    const auto actors = map_->actors();
    if (!shroud_.has_value() || transitions_ == nullptr)
        return;
    const model::MapActor& self = actors[actor];
    if (!self.owned() || (team_ >> self.owner & 1U) == 0)
        return;
    std::vector<model::Eye> others;
    for (std::uint32_t i = 0; i < actors.size(); ++i) {
        const model::MapActor& other = actors[i];
        if (i == actor || !other.owned() || (team_ >> other.owner & 1U) == 0)
            continue;
        const model::Eye eye = eye_of(other, at_[i]);
        if (eye.radius > 0)
            others.push_back(eye);
    }
    shroud_ = shroud_->moved(map_->terrain(), eye_of(self, from), eye_of(self, at_[actor]), others, *transitions_);
}

}
