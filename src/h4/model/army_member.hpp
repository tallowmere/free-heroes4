#pragma once

#include <optional>

#include "h4/model/alignment.hpp"

namespace h4::formats {
struct ArmySlot;
}

namespace h4::model {

class CreatureCatalog;

[[nodiscard]] std::optional<Alignment> member_alignment(const formats::ArmySlot& slot, const CreatureCatalog& creatures);

}
