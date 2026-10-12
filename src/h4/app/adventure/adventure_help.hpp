#pragma once

#include "h4/ui/help.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
struct ArmySlot;
}
namespace h4::model {
class GameDatabase;
struct MapActor;
}

namespace h4::app {

[[nodiscard]] ui::Help ring_help(const assets::Assets& assets, const model::GameDatabase* database, const formats::ArmySlot& slot);

[[nodiscard]] ui::Help army_row_help(const assets::Assets& assets, const model::GameDatabase* database, const model::MapActor& army);

[[nodiscard]] ui::Help army_move_help(const assets::Assets& assets, int tiles);

}
