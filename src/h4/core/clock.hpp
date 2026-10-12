#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace h4::core {

struct FrameClock {
    using duration   = std::chrono::milliseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<FrameClock, duration>;
};

using Instant = FrameClock::time_point;

class Timer {
public:
    static constexpr std::chrono::milliseconds kShortest{20};

    Timer() noexcept = default;
    explicit Timer(std::chrono::milliseconds interval) noexcept : interval_{floored(interval)} {
    }

    [[nodiscard]] std::chrono::milliseconds interval() const noexcept {
        return interval_;
    }
    void set_interval(std::chrono::milliseconds interval) noexcept {
        interval_ = floored(interval);
    }

    [[nodiscard]] bool take(Instant now) noexcept {
        if (!due_)
            due_ = now + interval_;
        if (interval_.count() <= 0 || now < *due_)
            return false;
        *due_ += interval_;
        return true;
    }

    void restart() noexcept {
        due_.reset();
    }

private:
    [[nodiscard]] static constexpr std::chrono::milliseconds floored(std::chrono::milliseconds interval) noexcept {
        return interval.count() <= 0 ? std::chrono::milliseconds{0} : std::max(interval, kShortest);
    }

    std::chrono::milliseconds interval_{0};
    std::optional<Instant> due_;
};

class Since {
public:
    [[nodiscard]] std::chrono::milliseconds age(Instant now) noexcept {
        if (!since_)
            since_ = now;
        return now - *since_;
    }
    void restart() noexcept {
        since_.reset();
    }

private:
    std::optional<Instant> since_;
};

[[nodiscard]] constexpr std::size_t
cycle(std::chrono::milliseconds at, std::chrono::milliseconds step, std::uint64_t phase, std::size_t count) noexcept {
    if (count == 0)
        return 0;
    const std::uint64_t steps = step.count() <= 0 || at.count() <= 0 ? 0 : static_cast<std::uint64_t>(at / step);
    return static_cast<std::size_t>((steps + phase) % count);
}

}
