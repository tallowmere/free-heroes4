#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <vector>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/spell_catalog.hpp"

namespace h4::model {
class Roller;
}

namespace h4::rules {

using KnownSpells = std::bitset<model::kSpellIds>;
using SpellUses = std::array<std::int32_t, model::kSpellIds>;

[[nodiscard]] std::optional<model::SpellId> pick_shrine_spell(const model::SpellCatalog& spells,
                                                              model::Alignment school,
                                                              std::int32_t level,
                                                              const formats::SpellMask& mask,
                                                              SpellUses& uses,
                                                              model::Roller& roller);

[[nodiscard]] std::optional<model::SpellId> school_gift(model::Alignment school, const KnownSpells& known, model::Roller& roller);

[[nodiscard]] bool may_learn(std::optional<std::int32_t> school_rank, std::int32_t spell_level) noexcept;

}
