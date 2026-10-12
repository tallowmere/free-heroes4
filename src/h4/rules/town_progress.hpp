#pragma once

#include <array>
#include <cstdint>

#include "h4/formats/scenario/objects.hpp"
#include "h4/model/alignment.hpp"

namespace h4::rules {

enum class BuildingFamily : std::uint8_t { misc, magic, creatures };

struct FamilyProgress {
    int built = 0;
    int total = 0;
};

[[nodiscard]] bool alignment_builds(model::Alignment alignment, int id) noexcept;

[[nodiscard]] std::array<FamilyProgress, 3>
building_progress(model::Alignment alignment, const formats::TownBuildings& buildings) noexcept;

}
