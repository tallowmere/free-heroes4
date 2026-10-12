#pragma once

#include <cstdint>

#include "h4/core/strong.hpp"

namespace h4::rules {

inline constexpr std::int32_t kDaysPerWeek  = 7;
inline constexpr std::int32_t kDaysPerMonth = 28;

using Day = core::Strong<struct DayTag, std::int32_t>;

class Calendar final {
public:
    constexpr Calendar() noexcept = default;
    constexpr explicit Calendar(Day day) noexcept : day_{day} {
    }

    [[nodiscard]] constexpr Day day() const noexcept {
        return day_;
    }
    [[nodiscard]] constexpr std::int32_t week() const noexcept {
        return (day_.value() - 1) / kDaysPerWeek + 1;
    }
    [[nodiscard]] constexpr std::int32_t month() const noexcept {
        return (day_.value() - 1) / kDaysPerMonth + 1;
    }
    [[nodiscard]] constexpr bool starts_week() const noexcept {
        return (day_.value() - 1) % kDaysPerWeek == 0;
    }
    [[nodiscard]] constexpr bool starts_month() const noexcept {
        return (day_.value() - 1) % kDaysPerMonth == 0;
    }
    [[nodiscard]] constexpr Calendar next() const noexcept {
        return Calendar{Day{day_.value() + 1}};
    }

    friend constexpr bool operator==(Calendar, Calendar) noexcept = default;

private:
    Day day_{1};
};

}
