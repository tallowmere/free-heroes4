#pragma once

#include <cstdint>

namespace h4::rules {

struct MapPoint {
    std::int32_t x     = 0;
    std::int32_t y     = 0;
    std::int32_t level = 0;
    bool bridge        = false;

    friend constexpr bool operator==(const MapPoint&, const MapPoint&) noexcept = default;
};

}
