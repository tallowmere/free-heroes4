#include "h4/model/shroud.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "h4/formats/transition_set.hpp"
#include "h4/model/map.hpp"
#include "h4/model/skills.hpp"

namespace h4::model {

std::int32_t sight_radius(const MapActor& actor, core::Size footprint) noexcept {
    const std::int32_t from_box = std::max(footprint.width, footprint.height) / 2 + 3;
    switch (actor.kind) {
    case ActorKind::town: return std::max(from_box, 15);
    case ActorKind::hero: {
        const formats::HeroRecord* leader = actor.army.leader();
        const int rank = leader == nullptr ? -1 : std::min(stored_rank(*leader, Discipline::scouting), 4);
        return 10 + rank;
    }
    case ActorKind::dwelling:
    case ActorKind::mine:
    case ActorKind::garrison: return from_box;
    default: return 0;
    }
}

std::int32_t Shroud::index(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept {
    if (x < 0 || x >= width_ || plane < 0 || plane >= planes_)
        return -1;
    const formats::GridSpan& span = spans_[static_cast<std::size_t>(x)];
    if (y < span.start || y >= span.stop)
        return -1;
    const std::size_t base = column_[static_cast<std::size_t>(plane * width_ + x)];
    return static_cast<std::int32_t>(base + static_cast<std::size_t>(y - span.start));
}

std::uint8_t Shroud::level(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept {
    const std::int32_t at = index(x, y, plane);
    return at < 0 ? std::uint8_t{2} : level_[static_cast<std::size_t>(at)];
}

std::span<const ShroudPiece> Shroud::record(std::int32_t x, std::int32_t y, std::int32_t plane) const noexcept {
    const std::int32_t at = index(x, y, plane);
    if (at < 0)
        return {};
    const std::uint32_t begin = first_[static_cast<std::size_t>(at)];
    const std::uint32_t end   = first_[static_cast<std::size_t>(at) + 1];
    return std::span<const ShroudPiece>{pieces_}.subspan(begin, end - begin);
}

template <class Visit>
void Shroud::each_in_disc(const formats::ScenarioGrid& grid, const Eye& eye, Visit&& visit) const {
    if (eye.radius <= 0)
        return;
    const std::int64_t cx = 2 * std::int64_t{eye.origin.x} + eye.footprint.width;
    const std::int64_t cy = 2 * std::int64_t{eye.origin.y} + eye.footprint.height;
    const std::int64_t r2 = 4 * std::int64_t{eye.radius} * eye.radius;

    const auto clamp32 = [](std::int64_t v) {
        return static_cast<std::int32_t>(std::clamp(v, std::int64_t{0}, std::int64_t{std::numeric_limits<std::int32_t>::max()}));
    };
    const std::int32_t left = std::max(clamp32((cx - 2 * eye.radius) / 2), 0);
    const std::int32_t top  = clamp32((cy - 2 * eye.radius) / 2);
    const std::int32_t right = static_cast<std::int32_t>(std::min<std::int64_t>((cx + 2 * eye.radius + 1) / 2 + 1, width_));
    const std::int32_t low = static_cast<std::int32_t>(std::min<std::int64_t>((cy + 2 * eye.radius + 1) / 2 + 1, width_));

    for (std::int32_t x = left; x < right; ++x) {
        const formats::GridSpan& span = grid.columns()[static_cast<std::size_t>(x)];
        for (std::int32_t y = std::max(top, span.start); y < std::min(low, span.stop); ++y) {
            const std::int64_t dx = 2 * std::int64_t{x} - cx + 1;
            const std::int64_t dy = 2 * std::int64_t{y} - cy + 1;
            if (dx * dx + dy * dy > r2)
                continue;
            if (const std::int32_t at = index(x, y, eye.plane); at >= 0)
                visit(static_cast<std::size_t>(at));
        }
    }
}

void Shroud::reveal(const formats::ScenarioGrid& grid, const Eye& eye) {
    each_in_disc(grid, eye, [this](std::size_t at) {
        if (level_[at] < kFogVisible)
            level_[at] = kFogVisible;
    });
}

Shroud Shroud::moved(const formats::ScenarioGrid& grid,
                     const Eye& from,
                     const Eye& to,
                     std::span<const Eye> others,
                     const formats::TransitionLibrary& transitions) const {
    Shroud out = *this;
    std::vector<bool> watched(out.level_.size(), false);
    for (const Eye& eye : others)
        each_in_disc(grid, eye, [&](std::size_t at) { watched[at] = true; });
    out.each_in_disc(grid, from, [&](std::size_t at) {
        if (out.level_[at] == kFogVisible && !watched[at])
            out.level_[at] = kFogFogged;
    });
    out.reveal(grid, to);
    out.finish(transitions);
    return out;
}

Shroud Shroud::at_start(const Map& map,
                        std::optional<PlayerColour> viewer,
                        std::uint8_t team,
                        const FootprintOf& footprint,
                        const formats::TransitionLibrary& transitions) {
    const formats::ScenarioGrid& grid = map.terrain();

    Shroud out;
    out.width_  = grid.width();
    out.planes_ = grid.levels();
    out.seed_   = map.terrain_seed();
    out.spans_.assign(grid.columns().begin(), grid.columns().end());

    out.column_.reserve(static_cast<std::size_t>(out.planes_ * out.width_) + 1);
    std::size_t total = 0;
    for (std::int32_t plane = 0; plane < out.planes_; ++plane)
        for (std::int32_t x = 0; x < out.width_; ++x) {
            out.column_.push_back(total);
            total += static_cast<std::size_t>(out.spans_[static_cast<std::size_t>(x)].size());
        }
    out.column_.push_back(total);
    out.level_.assign(total, kFogVisible);

    const int group = viewer ? int{std::to_underlying(*viewer)} : -1;
    for (std::int32_t plane = 0; plane < out.planes_; ++plane)
        for (std::int32_t x = 0; x < out.width_; ++x) {
            const formats::GridSpan& span = out.spans_[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; y < span.stop; ++y) {
                const formats::GridTile* tile = grid.tile(x, y, plane);
                if (tile == nullptr)
                    continue;
                const std::int32_t at = out.index(x, y, plane);
                if (at < 0)
                    continue;
                out.level_[static_cast<std::size_t>(at)] = static_cast<std::uint8_t>(formats::tile_fog(tile->fog, group));
            }
        }

    const std::uint8_t shared = team_mask(viewer, team);
    for (const MapActor& actor : map.actors()) {
        if (!actor.owned())
            continue;
        if ((shared >> actor.owner & 1U) == 0)
            continue;
        const core::Size box      = footprint ? footprint(actor) : core::Size{1, 1};
        const std::int32_t radius = sight_radius(actor, box);
        out.reveal(grid, Eye{actor.position, box, actor.level, radius});
    }

    out.finish(transitions);
    return out;
}

void Shroud::finish(const formats::TransitionLibrary& transitions) {
    any_ = std::ranges::any_of(level_, [](std::uint8_t v) { return v < kFogVisible; });
    pieces_.clear();
    first_.clear();
    if (any_)
        build_records(transitions);
    else
        first_.assign(level_.size() + 1, 0);
}

}
