#include "h4/screens/adventure/creature_idle.hpp"

#include <algorithm>
#include <utility>

#include "h4/formats/combat_actor.hpp"
#include "h4/formats/frame_rate.hpp"
#include "h4/model/roller.hpp"

namespace h4::screens {

namespace {

using formats::CombatState;

static_assert(CreatureIdle::kStates == formats::kCombatStates, "one entry per combat state");

constexpr std::array<CombatState, 10> kDraws{
    CombatState::block,
    CombatState::cast_spell,
    CombatState::fidget,
    CombatState::flinch,
    CombatState::melee,
    CombatState::ranged,
    CombatState::wait,
    CombatState::prewalk,
    CombatState::prewalk,
    CombatState::prewalk,
};

constexpr std::size_t index(CombatState state) noexcept {
    return std::to_underlying(state);
}

}

CreatureIdle::CreatureIdle(Model model) : model_{model} {
    enter(CombatState::wait);
}

bool CreatureIdle::has(CombatState state) const noexcept {
    return model_.frames[index(state)] > 0;
}

std::size_t CreatureIdle::frames(CombatState state) const noexcept {
    return model_.frames[index(state)];
}

std::uint32_t CreatureIdle::rate(CombatState state) const noexcept {
    return model_.rates[index(state)];
}

void CreatureIdle::enter(CombatState asked) {
    asked_ = asked;
    state_ = has(asked) ? asked : CombatState::base_frame;
    frame_ = 0;
    const std::uint32_t per_second = std::max<std::uint32_t>(rate(state_), 1);
    const std::size_t count        = frames(state_);
    loops_                         = 1;
    if ((asked == CombatState::walk || asked == CombatState::wait) && count > 0) {
        const std::uint32_t n = asked == CombatState::walk ? per_second * 3 : per_second;
        loops_                = static_cast<std::uint32_t>((count - 1 + std::size_t{2} * n) / count);
    }
    timer_.set_interval(std::chrono::milliseconds{formats::frame_ms_of(per_second)});
}

void CreatureIdle::tick(model::Roller& seed) {
    const CombatState current = state_;
    if (frame_ + 1 < frames(current)) {
        ++frame_;
        return;
    }
    if (current != CombatState::prewalk && current != CombatState::walk && current != CombatState::base_frame &&
        has(CombatState::base_frame)) {
        state_ = CombatState::base_frame;
        frame_ = 0;
        return;
    }
    if (--loops_ > 0) {
        if (current == CombatState::base_frame && has(asked_))
            state_ = asked_;
        frame_ = 0;
        return;
    }
    if (current == CombatState::prewalk)
        return enter(CombatState::walk);
    if (current == CombatState::walk)
        return enter(CombatState::postwalk);
    CombatState chosen = kDraws[seed.next_index(kDraws.size())];
    if (asked_ != CombatState::wait || !has(chosen))
        chosen = CombatState::wait;
    enter(chosen);
}

bool CreatureIdle::advance(core::Instant now, model::Roller& seed) {
    bool changed = false;
    while (timer_.take(now)) {
        const CombatState state = state_;
        const std::size_t frame = frame_;
        tick(seed);
        changed = changed || state != state_ || frame != frame_;
    }
    return changed;
}

}
