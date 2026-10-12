#pragma once

#include <cstddef>
#include <cstdint>

#include "h4/model/msvc_rand.hpp"

namespace h4::model {

class Roller {
public:
    explicit Roller(std::uint32_t state) noexcept : state_{state} {
    }

    [[nodiscard]] std::int32_t next(std::int32_t modulo) noexcept {
        state_ = msvc_rand_step(state_);
        return modulo <= 1 ? 0 : static_cast<std::int32_t>(drawn(static_cast<std::uint32_t>(modulo)));
    }
    [[nodiscard]] std::size_t next_index(std::size_t count) noexcept {
        state_ = msvc_rand_step(state_);
        return count <= 1 ? 0 : drawn(count);
    }
    [[nodiscard]] std::int32_t next_twice(std::int32_t modulo) noexcept {
        state_ = msvc_rand_step_twice(state_);
        return modulo <= 1 ? 0 : static_cast<std::int32_t>(drawn(static_cast<std::uint32_t>(modulo)));
    }

    [[nodiscard]] friend bool operator==(const Roller&, const Roller&) noexcept = default;

private:
    template <class Count> [[nodiscard]] Count drawn(Count count) const noexcept {
        return static_cast<Count>(msvc_rand_value(state_)) % count;
    }

    std::uint32_t state_;
};

}
