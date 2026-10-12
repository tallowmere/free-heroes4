#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "h4/rules/adventure_board.hpp"
#include "h4/rules/step_test.hpp"

namespace h4::rules {

struct ZoneCell {
    bool marked = false;
    bool restricted  = false;
    std::int8_t dir  = 0;
    std::int8_t cost = 0;
};

struct ZoneEnemy {
    MapPoint at;
    bool walked_past = false;
};

class EnemyZones {
public:
    [[nodiscard]] static EnemyZones mark(const StepTest& searcher, std::span<const ZoneEnemy> enemies);

    [[nodiscard]] ZoneCell at(const MapPoint& at) const noexcept;

private:
    explicit EnemyZones(const AdventureBoard& board);

    void walk(const StepTest& searcher, const MapPoint& enemy);
    bool offer(std::int32_t slot, const ZoneCell& record);

    const AdventureBoard* board_;
    std::vector<ZoneCell> cells_;
};

[[nodiscard]] constexpr bool zone_takes(const ZoneCell& old, const ZoneCell& record) noexcept {
    if (!old.marked)
        return true;
    if (record.restricted != old.restricted)
        return record.restricted;
    return record.cost < old.cost;
}

}
