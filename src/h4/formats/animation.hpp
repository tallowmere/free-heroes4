#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::formats {

class Animation {
public:
    [[nodiscard]] static core::Result<Animation> parse(std::span<const std::byte> data);

    [[nodiscard]] const BitmapGroup& group() const noexcept {
        return group_;
    }

    [[nodiscard]] std::uint32_t frame_ms() const noexcept {
        return frame_ms_;
    }

    [[nodiscard]] std::span<const std::uint32_t> schedule() const noexcept {
        return schedule_;
    }

    [[nodiscard]] const Layer* layer_at(std::chrono::milliseconds at, std::uint64_t phase) const noexcept;

    [[nodiscard]] bool anchored_x() const noexcept {
        return flag_a_;
    }
    [[nodiscard]] bool anchored_y() const noexcept {
        return flag_b_;
    }
    [[nodiscard]] std::uint32_t anchor_x() const noexcept {
        return dword_a_;
    }
    [[nodiscard]] std::uint32_t anchor_y() const noexcept {
        return dword_b_;
    }

private:
    explicit Animation(BitmapGroup group) : group_{std::move(group)} {
    }

    BitmapGroup group_;
    std::vector<std::uint32_t> schedule_;
    std::uint32_t frame_ms_ = 0;
    std::uint32_t dword_a_  = 0;
    std::uint32_t dword_b_  = 0;
    bool flag_a_            = false;
    bool flag_b_            = false;
};

}
