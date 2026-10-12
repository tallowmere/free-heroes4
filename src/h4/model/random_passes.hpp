#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/model/alignment.hpp"
#include "h4/model/artifact_catalog.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/roller.hpp"

namespace h4::model {

struct MapActor;
class HeroTable;
struct HeroRow;
struct RandomResolution;

[[nodiscard]] std::optional<Alignment> alignment_constraint(const MapActor& actor, const RandomResolution& choices);

[[nodiscard]] std::optional<Alignment> owner_alignment(const MapActor& actor, const RandomResolution& choices);

[[nodiscard]] int sub_level(std::string_view sub) noexcept;

void resolve_random_monster(MapActor& actor, const RandomResolution& choices, Roller& roller, bool on_water);

void resolve_random_dwelling(MapActor& actor, const RandomResolution& choices, Roller& roller);

template <class Id, class UseOf>
[[nodiscard]] std::pair<int, std::vector<Id>> least_used(std::span<const Id> candidates, UseOf&& use_of) {
    int lowest = std::numeric_limits<int>::max();
    std::vector<Id> ties;
    for (const Id id : candidates) {
        const int use = use_of(id);
        if (use < lowest) {
            lowest = use;
            ties.clear();
        }
        if (use == lowest)
            ties.push_back(id);
    }
    return {lowest, std::move(ties)};
}

[[nodiscard]] constexpr std::optional<std::size_t> nth_set_bit(std::uint32_t mask, int n) noexcept {
    for (std::size_t bit = 0; bit < 32; ++bit)
        if ((mask >> bit & 1U) != 0 && n-- == 0)
            return bit;
    return std::nullopt;
}

class ArtifactPool {
public:
    explicit ArtifactPool(const ArtifactCatalog* artifacts) noexcept : artifacts_{artifacts} {
    }

    void resolve(MapActor& actor, Roller& roller);
    [[nodiscard]] const ArtifactType* pick(Roller& roller, ArtifactRank rank) {
        return draw(roller, rank, false, {});
    }

private:
    [[nodiscard]] const ArtifactType* draw(Roller& roller, ArtifactRank rank, bool potion, std::span<const ArtifactId> fixed);

    const ArtifactCatalog* artifacts_;
    std::vector<bool> used_;
};

class HeroPool {
public:
    HeroPool(const HeroTable* heroes, std::span<const MapActor> actors);

    void resolve(MapActor& actor, const RandomResolution& choices, Roller& roller);

private:
    [[nodiscard]] std::optional<std::size_t> draw(std::optional<HeroClassId> hero_class, int sex, Roller& roller);
    [[nodiscard]] const HeroRow* named_row(int registry_id) const noexcept;

    const HeroTable* heroes_;
    std::vector<int> uses_;
};

}
