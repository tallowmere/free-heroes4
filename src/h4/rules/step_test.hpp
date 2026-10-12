#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>

#include "h4/rules/adventure_board.hpp"
#include "h4/rules/adventure_step.hpp"

namespace h4::rules {

class EnemyZones;

struct GuardGround {
    MapPoint centre;
    std::int32_t reach = 0;
    bool by_steps      = false;

    [[nodiscard]] static GuardGround of(const MapPoint& centre, std::int32_t kind, std::int32_t radius) noexcept {
        const std::int32_t r = (kind == 0 || radius > 3) ? 3 : radius;
        return GuardGround{centre, r + 1, kind == 1};
    }

    [[nodiscard]] bool allows(const MapPoint& from, const MapPoint& to, std::int32_t steps) const noexcept;
};

struct SearchRules {
    OwnsStep owns = owns_every_step;
    std::function<bool(std::uint32_t)> blocks;
    std::function<bool(const MapPoint&)> hidden;
    const EnemyZones* zones = nullptr;
    std::int32_t bound = std::numeric_limits<std::int32_t>::max();
    bool record_walled = false;
    std::optional<GuardGround> ground;
};

class StepTest {
public:
    StepTest(const AdventureBoard& board, const SearchRules& rules) noexcept : board_{&board}, rules_{&rules} {
    }

    [[nodiscard]] const AdventureBoard& board() const noexcept {
        return *board_;
    }
    [[nodiscard]] const SearchRules& rules() const noexcept {
        return *rules_;
    }

    [[nodiscard]] bool blocked(const MapPoint& at) const;
    [[nodiscard]] bool hidden(const MapPoint& at) const;

    struct Neighbour {
        MapPoint to;
        bool ordinary = false;
        bool shut     = false;
    };
    [[nodiscard]] std::optional<Neighbour> neighbour(const MapPoint& from, int dir) const;

private:
    const AdventureBoard* board_;
    const SearchRules* rules_;
};

}
