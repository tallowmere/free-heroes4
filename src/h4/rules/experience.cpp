#include "h4/rules/experience.hpp"

#include <algorithm>
#include <cmath>

namespace h4::rules {

namespace {

const std::vector<std::int64_t>& table() {
    static const std::vector<std::int64_t> levels = [] {
        std::vector<std::int64_t> t{0, 1000, 2000};
        while (t.size() < static_cast<std::size_t>(kMaxLevel)) {
            const std::int64_t a = t[t.size() - 1], b = t[t.size() - 2];
            t.push_back(a - 100 * static_cast<std::int64_t>(std::trunc(static_cast<double>(a - b) * -0.012)));
        }
        return t;
    }();
    return levels;
}

}

std::int64_t experience_for_level(std::int32_t level) {
    if (level < 2)
        return 0;
    level = std::min(level, kMaxLevel);
    return table()[static_cast<std::size_t>(level - 1)];
}

std::int32_t level_for_experience(std::int64_t experience) {
    const auto& t = table();
    return static_cast<std::int32_t>(std::upper_bound(t.begin(), t.end(), experience) - t.begin());
}

std::vector<std::int64_t> split_award(std::int64_t amount, std::span<const std::int32_t> levels, bool even) {
    std::vector<std::int64_t> shares(levels.size(), 0);
    std::int64_t total = 0;
    for (const std::int32_t level : levels)
        total += even ? 1 : level + 2;
    std::int64_t remaining = amount;
    for (std::size_t i = 0; i < levels.size() && total > 0; ++i) {
        const std::int64_t weight = even ? 1 : levels[i] + 2;
        const auto share          = static_cast<std::int64_t>(
            std::trunc(static_cast<double>(weight * remaining) / static_cast<double>(total) + 0.5)
        );
        shares[i] = share;
        remaining -= share;
        total -= weight;
    }
    return shares;
}

}
