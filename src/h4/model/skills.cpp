#include "h4/model/skills.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/hero_class.hpp"

namespace h4::model {

static_assert(kDisciplines == formats::kHeroSkills, "a hero record stores one rank per discipline");

namespace {

constexpr std::array<std::string_view, kDisciplines> kDisciplineKeywords{
    "tactics",     "toughness", "scouting",   "nobility",   "life",       "order",      "death",       "chaos",
    "nature",      "offense",   "defense",    "leadership", "combat",     "archery",    "resistance",  "pathfinding",
    "seamanship",  "stealth",   "estates",    "mining",     "diplomacy",  "healing",    "spirit",      "resurrection",
    "enchantment", "mind",      "charm",      "black",      "demonology", "necromancy", "conjuration", "pyromancy",
    "sorcery",     "herbalism", "meditation", "summoning",
};

constexpr std::array<std::string_view, kRanks> kRankKeywords{"basic", "advanced", "expert", "master", "grandmaster"};

constexpr std::array<std::string_view, kSchools> kSchoolIcons{"tactics", "combat", "scouting", "nobility", "life", "order", "death", "chaos", "nature"};

constexpr std::size_t kFirstMagicHead = std::to_underlying(Discipline::life);
static_assert(std::to_underlying(Discipline::nature) - kFirstMagicHead + 1 == kMagicAlignments);

}

std::string_view discipline_keyword(Discipline discipline) noexcept {
    return kDisciplineKeywords[std::to_underlying(discipline)];
}

std::string_view rank_keyword(int rank) noexcept {
    return rank >= 0 && rank < kRanks ? kRankKeywords[static_cast<std::size_t>(rank)] : std::string_view{};
}

Discipline school_head(Discipline discipline) noexcept {
    const std::size_t index = std::to_underlying(discipline);
    return index < kSchools ? discipline : kAllDisciplines[(index - kSchools) / 3];
}

std::optional<Alignment> magic_school(Discipline head) noexcept {
    const std::size_t index = std::to_underlying(head);
    return index >= kFirstMagicHead && index < kSchools ? alignment_of(static_cast<std::int32_t>(index - kFirstMagicHead)) :
                                                          std::nullopt;
}

std::string_view school_icon_name(Discipline discipline) noexcept {
    return kSchoolIcons[std::to_underlying(school_head(discipline))];
}

int stored_rank(const formats::HeroRecord& hero, Discipline discipline) noexcept {
    const std::uint8_t rank = hero.skills[std::to_underlying(discipline)];
    return rank == formats::kNoSkill ? -1 : int{rank};
}

std::vector<HeroSkill> hero_skills(const formats::HeroRecord& hero) {
    std::vector<HeroSkill> out;
    for (const Discipline d : kAllDisciplines)
        if (const int rank = stored_rank(hero, d); rank >= 0)
            out.push_back({d, rank});
    if (!out.empty())
        return out;
    if (const HeroClass* klass = hero_class(hero))
        for (const Discipline d : klass->disciplines())
            out.push_back({d, 0});
    return out;
}

int hero_level(const formats::HeroRecord& hero) {
    if (hero.level > 0)
        return hero.level;
    constexpr int kSkillLevelCap = 40;
    int level                    = 1;
    for (const HeroSkill& skill : hero_skills(hero))
        level += skill.rank + 1;
    if (const HeroClass* klass = hero_class(hero))
        level -= static_cast<int>(klass->disciplines().size());
    return std::clamp(level, 1, kSkillLevelCap);
}

std::vector<HeroSkill> panel_order(std::vector<HeroSkill> skills) {
    std::array<int, kSchools> totals{};
    for (const HeroSkill& skill : skills)
        totals[std::to_underlying(school_head(skill.discipline))] += skill.rank + 1;
    std::ranges::stable_sort(skills, [&](const HeroSkill& a, const HeroSkill& b) {
        const Discipline ha = school_head(a.discipline), hb = school_head(b.discipline);
        const int ta = totals[std::to_underlying(ha)], tb = totals[std::to_underlying(hb)];
        if (ta != tb)
            return ta > tb;
        if (ha != hb)
            return ha < hb;
        return a.discipline < b.discipline;
    });
    return skills;
}

}
