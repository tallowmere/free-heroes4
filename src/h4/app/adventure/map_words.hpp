#pragma once

#include <cstdint>
#include <optional>

#include "h4/scene/map_hover.hpp"
#include "h4/ui/help.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::model {
class GameDatabase;
class Map;
class Shroud;
}

namespace h4::app {

[[nodiscard]] ui::Help map_words(const assets::Assets& assets,
                                 const model::GameDatabase* database,
                                 const model::Map& map,
                                 const model::Shroud* seen,
                                 const scene::MapHover& hover,
                                 std::optional<std::uint32_t> object,
                                 bool show_coordinates);

}
