#pragma once

#include <cstdint>

#include "h4/screens/adventure/treasure_chest_dialog.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::model {
class AdventureTexts;
struct ArtifactType;
struct MapActor;
struct Spell;
}
namespace h4::rules {
struct ShrineLesson;
}

namespace h4::app {

[[nodiscard]] screens::BasicDialogContent shrine_words(const assets::Assets& assets,
                                                       const model::AdventureTexts& texts,
                                                       const model::MapActor& shrine,
                                                       const model::Spell& spell,
                                                       const rules::ShrineLesson& lesson,
                                                       const model::MapActor& army);

[[nodiscard]] screens::TreasureChestContent
chest_words(const assets::Assets& assets, const model::AdventureTexts& texts, const model::MapActor& chest, std::int32_t gold);

[[nodiscard]] screens::BasicDialogContent artifact_words(const assets::Assets& assets,
                                                         const model::AdventureTexts& texts,
                                                         const model::MapActor& chest,
                                                         const model::ArtifactType& artifact);

}
