#pragma once

#include "h4/formats/scenario/records.hpp"
#include "h4/model/creature_catalog.hpp"

namespace h4::rules {

[[nodiscard]] int strength(const formats::ArmyRecord& army, const model::CreatureCatalog& creatures);

[[nodiscard]] int alignment_relation(model::Alignment a, model::Alignment b) noexcept;

[[nodiscard]] int member_morale(const formats::ArmyRecord& army, std::size_t slot, const model::CreatureCatalog& creatures) noexcept;

}
