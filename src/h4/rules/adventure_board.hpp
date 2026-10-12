#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/rules/map_point.hpp"

namespace h4::formats {
class AdvObject;
class ScenarioGrid;
class TransitionLibrary;
}
namespace h4::model {
class Map;
}

namespace h4::rules {

struct BoardTile {
    std::uint8_t blocked = 0;
    std::uint8_t bridge  = 0;
    std::uint8_t span    = 0;
    std::uint8_t ramp_y  = 0;
    std::uint8_t ramp_x  = 0;
    std::array<std::uint8_t, 2> wall_y{};
    std::array<std::uint8_t, 2> wall_x{};
    std::array<std::uint8_t, 2> passage_y{};
    std::array<std::uint8_t, 2> passage_x{};
};

class AdventureBoard {
public:
    [[nodiscard]] static AdventureBoard
    build(const model::Map& map, std::span<const formats::AdvObject* const> models, const formats::TransitionLibrary* transitions);

    [[nodiscard]] const formats::ScenarioGrid& grid() const noexcept;

    [[nodiscard]] std::size_t tile_count() const noexcept {
        return tiles_.size();
    }
    [[nodiscard]] std::int32_t index(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;
    [[nodiscard]] MapPoint at_index(std::int32_t index) const noexcept;
    [[nodiscard]] std::int32_t slot(const MapPoint& at) const noexcept {
        const std::int32_t tile = index(at.x, at.y, at.level);
        return tile < 0 ? -1 : 2 * tile + (at.bridge ? 1 : 0);
    }
    [[nodiscard]] MapPoint at_slot(std::int32_t slot) const noexcept {
        MapPoint out = at_index(slot / 2);
        out.bridge   = (slot % 2) != 0;
        return out;
    }

    [[nodiscard]] const BoardTile* tile(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> objects(const MapPoint& at) const noexcept;

    [[nodiscard]] bool blocked(const MapPoint& at) const noexcept;

    [[nodiscard]] bool dry_land(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept;
    [[nodiscard]] bool at_sea(const MapPoint& at) const noexcept {
        return !at.bridge && !dry_land(at.x, at.y, at.level);
    }

    [[nodiscard]] const formats::AdvObject* model(std::uint32_t actor) const noexcept;

    [[nodiscard]] bool is_army(std::uint32_t actor) const noexcept;
    [[nodiscard]] bool owns_steps(std::uint32_t actor) const noexcept;

    void move_army(std::uint32_t actor, const MapPoint& to);
    void remove(std::uint32_t actor);
    [[nodiscard]] MapPoint origin(std::uint32_t actor) const noexcept;

private:
    explicit AdventureBoard(const model::Map& map);

    void stamp(std::uint32_t actor, const formats::AdvObject* model, const MapPoint& origin, int sign);
    void list(std::uint32_t actor, std::int32_t index, bool bridge);
    void flatten();

    const model::Map* map_;
    std::vector<BoardTile> tiles_;
    std::vector<bool> dry_;
    std::vector<const formats::AdvObject*> models_;
    std::vector<std::array<std::int16_t, 3>> where_;
    struct Range {
        std::uint32_t first = 0;
        std::uint32_t count = 0;
    };
    std::vector<Range> lists_;
    std::vector<std::uint32_t> listed_;
    std::vector<std::pair<std::int32_t, std::uint32_t>> entries_;
    std::vector<MapPoint> origins_;
};

}
