#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/alignment.hpp"

namespace h4::model {

enum class Discipline : std::uint8_t {
    tactics,
    toughness,
    scouting,
    nobility,
    life,
    order,
    death,
    chaos,
    nature,
    offense,
    defense,
    leadership,
    combat,
    archery,
    resistance,
    pathfinding,
    seamanship,
    stealth,
    estates,
    mining,
    diplomacy,
    healing,
    spirit,
    resurrection,
    enchantment,
    mind,
    charm,
    black,
    demonology,
    necromancy,
    conjuration,
    pyromancy,
    sorcery,
    herbalism,
    meditation,
    summoning,
};

inline constexpr std::size_t kDisciplines = 36;
inline constexpr std::size_t kSchools     = 9;
inline constexpr int kRanks               = 5;
static_assert(std::to_underlying(Discipline::summoning) + 1 == kDisciplines);

inline constexpr std::array<Discipline, kDisciplines> kAllDisciplines = [] {
    std::array<Discipline, kDisciplines> out{};
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<Discipline>(i);
    return out;
}();

[[nodiscard]] std::string_view discipline_keyword(Discipline discipline) noexcept;
[[nodiscard]] std::string_view rank_keyword(int rank) noexcept;

[[nodiscard]] Discipline school_head(Discipline discipline) noexcept;

[[nodiscard]] std::optional<Alignment> magic_school(Discipline head) noexcept;

[[nodiscard]] std::string_view school_icon_name(Discipline discipline) noexcept;

struct HeroSkill {
    Discipline discipline = Discipline::tactics;
    int rank              = 0;
};

[[nodiscard]] int stored_rank(const formats::HeroRecord& hero, Discipline discipline) noexcept;

[[nodiscard]] std::vector<HeroSkill> hero_skills(const formats::HeroRecord& hero);

[[nodiscard]] int hero_level(const formats::HeroRecord& hero);

[[nodiscard]] std::vector<HeroSkill> panel_order(std::vector<HeroSkill> skills);

}
