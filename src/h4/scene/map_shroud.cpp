#include "h4/scene/map_shroud.hpp"

#include <algorithm>
#include <array>

#include "h4/assets/assets.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/tiled_fill.hpp"
#include "h4/model/map.hpp"
#include "h4/scene/map_edges.hpp"
#include "h4/scene/map_lattice.hpp"

namespace h4::scene {

namespace {

constexpr std::array<std::uint8_t, 4> kLevelCoverage{15, 8, 0, 0};

[[nodiscard]] graphics::Surface565 fog_texture(const assets::Assets& assets) {
    const formats::BitmapGroup* group = assets.group(kShroudGroup);
    if (group == nullptr)
        return graphics::Surface565{core::Size{1, 1}};
    const formats::Layer* layer = group->find("background");
    if (layer == nullptr || layer->is_placeholder())
        return graphics::Surface565{core::Size{1, 1}};
    graphics::Surface565 out{layer->bounds().size()};
    const graphics::Canvas canvas{out};
    canvas.draw_layer(*layer, core::Point{} - layer->bounds().origin());
    return out;
}

}

MapShroud::MapShroud(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen)
: map_{&map}, transitions_{assets.transitions()}, seen_{seen}, fog_{fog_texture(assets)}, plane_{core::Size{}} {
}

bool MapShroud::active() const noexcept {
    return seen_ != nullptr && transitions_ != nullptr && seen_->any();
}

void MapShroud::draw_tile(graphics::CoverageSurface& surface,
                          std::int32_t x,
                          std::int32_t y,
                          std::int32_t level,
                          core::Point anchor,
                          const graphics::CornerLifts& lifts) const {
    const std::uint8_t mine                          = kLevelCoverage[seen_->level(x, y, level)];
    const std::span<const model::ShroudPiece> record = seen_->record(x, y, level);
    bool first = true;
    for (const model::ShroudPiece& piece : record) {
        const formats::TransitionSet* set = transitions_->slot(piece.slot);
        if (set == nullptr)
            continue;
        const formats::TransitionEntry* entry = set->entry(piece.entry);
        if (entry == nullptr)
            continue;
        graphics::blend_diamond_coverage(surface, anchor, lifts, *entry, kLevelCoverage[piece.level], first ? &mine : nullptr);
        first = false;
    }
    if (first)
        graphics::fill_diamond_coverage(surface, anchor, lifts, mine);
}

void MapShroud::paint_margins(core::Point origin, std::int32_t level) const {
    const formats::ScenarioGrid& grid = map_->terrain();
    const std::int32_t width          = grid.width();
    const std::int32_t half           = width / 2;
    const std::int32_t bottom         = map_box(grid).height();
    const core::Rect plane            = plane_.bounds();

    const bool top_shows    = origin.y < 0;
    const bool bottom_shows = bottom - kRaiseAllowance < origin.y + plane.height();
    if (!top_shows && !bottom_shows)
        return;

    graphics::CoverageSurface row{core::Size{kHalfTile, 1}};
    const auto fill = [&](std::int32_t c, const core::Rect& rect) {
        const core::Rect area = rect.translated(core::Point{} - origin).intersected(plane);
        for (std::int32_t y = area.top(); y < area.bottom(); ++y)
            for (std::int32_t x = area.left(); x < area.right(); ++x)
                plane_.set({x, y}, row.at({x + origin.x - c * kHalfTile, 0}));
    };
    const auto wedge = [&](std::int32_t c, const core::Rect& rect, graphics::WedgeCorner corner) {
        if (rect.empty())
            return;
        graphics::walk_wedge(rect.translated(core::Point{} - origin), {}, corner, plane, [&](const core::Rect& quadrant, core::Point) {
            fill(c, quadrant.translated(origin));
        });
    };
    const auto build_row = [&](std::int32_t c, core::Point tile) {
        row.fill(0);
        draw_tile(row, tile.x, tile.y, level, {c % 2 == 0 ? 0 : -kHalfTile, -15}, graphics::CornerLifts{});
    };

    const auto [first, last] = edge_columns(origin.x, origin.x + plane.width(), width);
    const auto band          = [&](std::int32_t c, const EdgeBand& edge) {
        fill(c, edge.flat);
        wedge(c, edge.wedge, edge.corner);
    };
    for (std::int32_t c = first; c < last; ++c) {
        const std::int32_t k = c / 2;
        if (top_shows) {
            build_row(c, {half - k - 1, k});
            band(c, top_edge_band(grid, c, level));
        }
        if (bottom_shows) {
            build_row(c, {width - k - 1, half + k});
            band(c, bottom_edge_band(grid, c, level));
        }
    }
}

void MapShroud::paint(const graphics::Canvas& canvas, core::Point camera, std::int32_t level) const {
    if (!active())
        return;
    const core::Rect clip = canvas.local_clip();
    if (clip.empty())
        return;

    if (plane_.size() != clip.size())
        plane_ = graphics::CoverageSurface{clip.size()};
    plane_.fill(kLevelCoverage[0]);

    const formats::ScenarioGrid& grid = map_->terrain();
    const std::int32_t width          = grid.width();
    const core::Point origin = camera + clip.origin();
    const core::Rect plane{{}, clip.size()};

    for (std::int32_t x = 0; x < width; ++x) {
        const formats::GridSpan& span = grid.columns()[static_cast<std::size_t>(x)];
        for (std::int32_t y = span.start; y < span.stop; ++y) {
            const core::Point anchor = box_anchor(x, y, width) - origin;
            if (!tile_reaches(anchor, plane))
                continue;

            draw_tile(plane_,
                      x,
                      y,
                      level,
                      anchor,
                      graphics::CornerLifts{grid.elevation(x + 1, y, level),
                                            grid.elevation(x, y, level),
                                            grid.elevation(x, y + 1, level),
                                            grid.elevation(x + 1, y + 1, level)});
        }
    }
    paint_margins(origin, level);

    for (std::int32_t y = 0; y < clip.height(); ++y)
        for (std::int32_t x = 0; x < clip.width(); ++x) {
            const std::uint8_t n = plane_.at({x, y});
            if (n == 0)
                continue;
            const graphics::Color565 art = graphics::texel_wrapped(fog_, origin + core::Point{x, y});
            canvas.blend_pixel({clip.left() + x, clip.top() + y}, art, graphics::coverage_weight(n));
        }
}

}
