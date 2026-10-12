#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "h4/rules/adventure_board.hpp"
#include "h4/rules/adventure_step.hpp"
#include "h4/rules/step_test.hpp"

namespace h4::rules {

class PathField {
public:
    using Rules = SearchRules;

    [[nodiscard]] static PathField
    search(const AdventureBoard& board, const MapPoint& source, const TerrainCosts& terrain, const Rules& rules);

    [[nodiscard]] const MapPoint& source() const noexcept {
        return source_;
    }

    [[nodiscard]] std::optional<std::int32_t> total(const MapPoint& to) const noexcept;

    struct Step {
        MapPoint from;
        int dir = 0;
    };
    [[nodiscard]] std::optional<Step> last_step(const MapPoint& to) const noexcept;

    struct PathStep {
        MapPoint at;
        int dir            = 0;
        std::int32_t total = 0;
    };
    [[nodiscard]] std::vector<PathStep> path_to(const MapPoint& to) const;

    [[nodiscard]] std::vector<MapPoint> reached() const;

private:
    PathField(const AdventureBoard& board, const MapPoint& source);

    [[nodiscard]] std::int32_t slot(const MapPoint& at) const noexcept;
    [[nodiscard]] MapPoint point(std::int32_t slot) const noexcept;

    const AdventureBoard* board_;
    MapPoint source_;
    std::vector<std::int32_t> total_;
    std::vector<std::int32_t> from_;
    std::vector<std::int8_t> dir_;
};

}
