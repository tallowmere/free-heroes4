#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace h4::rules {

inline constexpr std::int32_t kMaxLevel = 70;

[[nodiscard]] std::int64_t experience_for_level(std::int32_t level);

[[nodiscard]] std::int32_t level_for_experience(std::int64_t experience);

[[nodiscard]] std::vector<std::int64_t> split_award(std::int64_t amount, std::span<const std::int32_t> levels, bool even);

}
