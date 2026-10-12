#include "h4/cursor/animation_clock.hpp"

#include "h4/formats/animation.hpp"

namespace h4::cursor {

void AnimationClock::advance(core::Instant now) noexcept {
    if (animation_ == nullptr)
        return;
    at_ = start_.age(now);
}

void AnimationClock::reset() noexcept {
    at_ = std::chrono::milliseconds{0};
    start_.restart();
}

const formats::Layer* AnimationClock::current() const noexcept {
    return animation_ == nullptr ? nullptr : animation_->layer_at(at_, 0);
}

}
