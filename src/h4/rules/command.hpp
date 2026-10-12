#pragma once

#include <cstdint>
#include <variant>

#include "h4/core/strong.hpp"
#include "h4/rules/map_point.hpp"

namespace h4::rules {

using PlayerId = core::Strong<struct PlayerIdTag, std::uint8_t>;

namespace commands {

struct EndTurn {
    PlayerId player;
};

struct MoveArmy {
    PlayerId player;
    std::uint32_t army = 0;
    MapPoint to;
    std::int32_t cost = 0;
};

struct Visit {
    PlayerId player;
    std::uint32_t army   = 0;
    std::uint32_t object = 0;
    std::int32_t cost    = 0;
    bool experience = false;
};

struct FireEvent {
    PlayerId player;
    std::uint32_t event = 0;
};

}

using Command = std::variant<commands::EndTurn, commands::MoveArmy, commands::Visit, commands::FireEvent>;

}
