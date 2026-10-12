#include "h4/rules/town_progress.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace h4::rules {

namespace {

constexpr std::uint64_t kShared = (std::uint64_t{1} << 20) - 1;
constexpr std::uint64_t bits(std::initializer_list<int> ids) {
    std::uint64_t out = 0;
    for (int id : ids)
        out |= std::uint64_t{1} << id;
    return out;
}
constexpr std::uint64_t kGuilds = bits({20, 21, 22, 23, 24, 25, 26});
constexpr std::array<std::uint64_t, 6> kBuilds{
    kShared | kGuilds | bits({27, 28, 29}),
    kShared | kGuilds | bits({30, 31}),
    kShared | kGuilds | bits({32, 33}),
    kShared | kGuilds | bits({34, 35, 36}),
    kShared | kGuilds | bits({37, 38, 39}),
    kShared | bits({35, 40, 41, 42}),
};

constexpr std::array kMisc{0, 1, 2, 7, 8, 9, 10, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};
constexpr std::array kMagic{20, 21, 22, 23, 24, 25, 26};
constexpr std::array kCreatures{12, 13, 14, 15, 16, 17, 18, 19, 37};

FamilyProgress progress(model::Alignment alignment, const formats::TownBuildings& buildings, std::span<const int> ids, int maximum) noexcept {
    FamilyProgress out;
    for (int id : ids) {
        if (alignment_builds(alignment, id))
            ++out.total;
        if (buildings.has(static_cast<std::uint8_t>(id)))
            ++out.built;
    }
    out.total = std::min(out.total, maximum);
    return out;
}

}

bool alignment_builds(model::Alignment alignment, int id) noexcept {
    const auto index = std::to_underlying(alignment);
    return id >= 0 && id < formats::kBuildingIds && index < kBuilds.size() && (kBuilds[index] >> id & 1U) != 0;
}

std::array<FamilyProgress, 3> building_progress(model::Alignment alignment, const formats::TownBuildings& buildings) noexcept {
    return {progress(alignment, buildings, kMisc, 100),
            progress(alignment, buildings, kMagic, 100),
            progress(alignment, buildings, kCreatures, 6)};
}

}
