#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "h4/screens/adventure/army_window_view.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::model {
class GameDatabase;
struct MapActor;
}
namespace h4::rules {
struct HeroState;
}

namespace h4::app {

[[nodiscard]] screens::ArmyWindowModel army_window_content(const assets::Assets& assets,
                                                           const model::MapActor& army,
                                                           std::optional<std::size_t> selected,
                                                           const rules::HeroState* hero,
                                                           const model::GameDatabase* database,
                                                           std::int32_t spent);

}
