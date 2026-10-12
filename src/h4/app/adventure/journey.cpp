#include "h4/app/adventure/journey.hpp"

#include <utility>

namespace h4::app {

void Journey::begin(std::uint32_t actor, std::optional<Visit> visit, std::unique_ptr<ArmyMover> mover) {
    actor_   = actor;
    charged_ = 0;
    visit_   = std::move(visit);
    mover_   = std::move(mover);
    held_since_.reset();
}

void Journey::tick(std::uint32_t now, bool held, const ArmyMover::Arrive& arrive, const ArmyMover::TileHook& hook) {
    if (held) {
        if (!held_since_)
            held_since_ = now;
        return;
    }
    if (held_since_) {
        mover_->delay(now - *held_since_);
        held_since_.reset();
    }
    mover_->tick(now, arrive, hook);
}

std::optional<Journey::Rest> Journey::end() {
    if (!mover_)
        return std::nullopt;
    const Rest rest{mover_->point(), mover_->pose()};
    mover_.reset();
    return rest;
}

std::optional<Journey::Visit> Journey::visit_from(const rules::MapPoint& at) {
    const std::optional<Visit> visit = std::exchange(visit_, std::nullopt);
    if (visit.has_value() && visit->from == at)
        return visit;
    return std::nullopt;
}

}
