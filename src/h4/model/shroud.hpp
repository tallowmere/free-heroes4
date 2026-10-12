#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/model/player_colour.hpp"

namespace h4::formats {
class TransitionLibrary;
}

namespace h4::model {

class Map;
struct MapActor;

inline constexpr std::uint8_t kFogHidden  = 0;
inline constexpr std::uint8_t kFogFogged  = 1;
inline constexpr std::uint8_t kFogVisible = 2;

struct ShroudPiece {
    std::uint8_t level  = 0;
    std::uint16_t entry = 0;
    std::uint8_t slot   = 0;
};

using FootprintOf = std::function<core::Size(const MapActor&)>;
[[nodiscard]] std::int32_t sight_radius(const MapActor& actor, core::Size footprint) noexcept;

struct Eye {
    core::Point origin;
    core::Size footprint{1, 1};
    std::int32_t plane  = 0;
    std::int32_t radius = 0;
};

class Shroud {
public:
    [[nodiscard]] static constexpr std::uint8_t team_mask(std::optional<PlayerColour> viewer, std::uint8_t team) noexcept {
        if (team != 0)
            return team;
        return static_cast<std::uint8_t>(viewer ? 1U << std::to_underlying(*viewer) : 0U);
    }

    [[nodiscard]] static Shroud at_start(const Map& map,
                                         std::optional<PlayerColour> viewer,
                                         std::uint8_t team,
                                         const FootprintOf& footprint,
                                         const formats::TransitionLibrary& transitions);

    [[nodiscard]] Shroud moved(const formats::ScenarioGrid& grid,
                               const Eye& from,
                               const Eye& to,
                               std::span<const Eye> others,
                               const formats::TransitionLibrary& transitions) const;

    [[nodiscard]] std::uint8_t level(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept;
    [[nodiscard]] std::span<const ShroudPiece> record(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept;
    [[nodiscard]] bool any() const noexcept {
        return any_;
    }

private:
    Shroud() = default;

    [[nodiscard]] std::int32_t index(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept;

    template <class Visit> void each_in_disc(const formats::ScenarioGrid& grid, const Eye& eye, Visit&& visit) const;
    void reveal(const formats::ScenarioGrid& grid, const Eye& eye);
    void build_records(const formats::TransitionLibrary& transitions);
    void finish(const formats::TransitionLibrary& transitions);

    std::vector<formats::GridSpan> spans_;
    std::vector<std::size_t> column_;
    std::vector<std::uint8_t> level_;
    std::vector<ShroudPiece> pieces_;
    std::vector<std::uint32_t> first_;
    std::uint32_t seed_  = 0;
    std::int32_t width_  = 0;
    std::int32_t planes_ = 0;
    bool any_            = false;
};

}
