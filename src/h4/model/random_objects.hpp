#pragma once

#include <array>
#include <optional>

#include "h4/model/alignment.hpp"
#include "h4/model/scenario_summary.hpp"

namespace h4::model {

class ArtifactCatalog;
class CreatureCatalog;
class HeroTable;

struct RandomResolution {
    std::array<std::optional<Alignment>, kPlayerColours> alignments{};
    const CreatureCatalog* creatures = nullptr;
    const ArtifactCatalog* artifacts = nullptr;
    const HeroTable* heroes = nullptr;
    MapDifficulty difficulty = MapDifficulty::normal;
};

}
