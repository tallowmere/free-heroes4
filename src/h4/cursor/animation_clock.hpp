#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "h4/core/clock.hpp"

namespace h4::formats {
class Animation;
class Layer;
}

namespace h4::cursor {

class AnimationClock {
public:
    AnimationClock() noexcept = default;
    explicit AnimationClock(const formats::Animation& animation) noexcept : animation_{&animation} {
    }

    void advance(core::Instant now) noexcept;
    void reset() noexcept;

    [[nodiscard]] bool bound() const noexcept {
        return animation_ != nullptr;
    }

    [[nodiscard]] const formats::Layer* current() const noexcept;

private:
    const formats::Animation* animation_ = nullptr;
    std::chrono::milliseconds at_{0};
    core::Since start_;
};

}
