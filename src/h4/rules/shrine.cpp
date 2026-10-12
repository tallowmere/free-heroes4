#include "h4/rules/shrine.hpp"

#include <span>

#include "h4/model/random_passes.hpp"
#include "h4/model/roller.hpp"

namespace h4::rules {

namespace {

constexpr std::array<std::uint8_t, 3> kLife{8, 10, 58};
constexpr std::array<std::uint8_t, 3> kOrder{109, 115, 72};
constexpr std::array<std::uint8_t, 4> kDeath{23, 111, 125, 128};
constexpr std::array<std::uint8_t, 5> kChaos{13, 14, 44, 55, 71};
constexpr std::array<std::uint8_t, 5> kNature{104, 151, 169, 173, 177};

[[nodiscard]] std::span<const std::uint8_t> gifts_of(model::Alignment school) noexcept {
    switch (school) {
    case model::Alignment::life: return kLife;
    case model::Alignment::order: return kOrder;
    case model::Alignment::death: return kDeath;
    case model::Alignment::chaos: return kChaos;
    case model::Alignment::nature: return kNature;
    case model::Alignment::might: return {};
    }
    return {};
}

}

std::optional<model::SpellId> pick_shrine_spell(const model::SpellCatalog& spells,
                                                model::Alignment school,
                                                std::int32_t level,
                                                const formats::SpellMask& mask,
                                                SpellUses& uses,
                                                model::Roller& roller) {
    std::vector<model::SpellId> candidates;
    for (const model::Spell& spell : spells.all())
        if (spell.level == level && spell.school == school && spell.teachable)
            candidates.push_back(spell.id);
    std::vector<model::SpellId> narrowed;
    for (const model::SpellId id : candidates)
        if (mask.test(id.value()))
            narrowed.push_back(id);
    if (narrowed.empty())
        narrowed = candidates;
    if (narrowed.empty())
        return std::nullopt;

    const std::vector<model::SpellId> ties = model::least_used(std::span<const model::SpellId>{narrowed}, [&](model::SpellId id) {
                                                 return uses[id.value()];
                                             }).second;
    const model::SpellId chosen = ties[roller.next_index(ties.size())];
    ++uses[chosen.value()];
    return chosen;
}

std::optional<model::SpellId> school_gift(model::Alignment school, const KnownSpells& known, model::Roller& roller) {
    const std::span<const std::uint8_t> list = gifts_of(school);
    if (list.empty())
        return std::nullopt;
    for (const std::uint8_t id : list)
        if (known.test(id))
            return std::nullopt;
    return model::SpellId{list[roller.next_index(list.size())]};
}

bool may_learn(std::optional<std::int32_t> school_rank, std::int32_t spell_level) noexcept {
    return spell_level <= (school_rank ? *school_rank + 1 : 0);
}

}
