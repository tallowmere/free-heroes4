#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/model/player_colour.hpp"
#include "h4/model/shroud.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class TransitionLibrary;
}
namespace h4::model {
class Map;
struct MapActor;
}

namespace h4::scene {

class ViewerShroud {
public:
    ViewerShroud(const assets::Assets& assets, const model::Map& map, std::optional<model::PlayerColour> viewer, std::uint8_t team);

    [[nodiscard]] const model::Shroud* plane() const noexcept {
        return shroud_.has_value() ? &*shroud_ : nullptr;
    }

    void moved(std::uint32_t actor, core::Point cell, std::int32_t level);

private:
    struct Cell {
        core::Point cell;
        std::int32_t level                 = 0;
        bool operator==(const Cell&) const = default;
    };

    [[nodiscard]] model::Eye eye_of(const model::MapActor& actor, const Cell& at) const;

    const assets::Assets* assets_;
    const model::Map* map_;
    const formats::TransitionLibrary* transitions_ = nullptr;
    std::uint8_t team_                             = 0;
    std::vector<Cell> at_;
    std::optional<model::Shroud> shroud_;
};

}
