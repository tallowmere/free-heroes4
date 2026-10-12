#include "h4/scene/mini_map.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "h4/assets/assets.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/shade565.hpp"
#include "h4/model/map.hpp"
#include "h4/model/shroud.hpp"
#include "h4/scene/map_render.hpp"

namespace h4::scene {

namespace {

struct Rgb {
    std::uint8_t r, g, b;
};

constexpr std::array<Rgb, 28> kTerrainColours{{
    {83, 133, 183},  {65, 104, 143},
    {68, 96, 19},    {44, 103, 15},
    {122, 121, 99},  {176, 175, 144},
    {62, 65, 37},    {107, 112, 64},
    {131, 26, 13},   {53, 33, 47},
    {233, 236, 245}, {190, 203, 241},
    {222, 210, 145}, {252, 206, 96},
    {112, 88, 64},   {88, 76, 48},
    {165, 156, 107}, {216, 209, 182},
    {83, 133, 183},
    {223, 47, 0},
    {190, 203, 241},
    {105, 107, 121},
    {186, 127, 211},
    {104, 83, 211},
    {31, 30, 45},
    {155, 38, 38},
    {62, 197, 80},
    {144, 64, 15},
}};

constexpr std::array<Rgb, 7> kPlayerColours{{
    {230, 60, 60},
    {22, 51, 224},
    {20, 160, 20},
    {248, 157, 13},
    {210, 70, 210},
    {70, 220, 220},
    {160, 160, 160},
}};

constexpr std::array<std::string_view, 13> kOwnable{
    "town",
    "random_town",
    "mine",
    "garrison",
    "creature_dwelling",
    "random_creature_dwelling",
    "windmill",
    "shipyard",
    "lighthouse",
    "weekly_material_generator",
    "random_weekly_material_generator",
    "sanctuary",
    "outpost",
};

[[nodiscard]] graphics::Color565 to565(Rgb c) noexcept {
    return graphics::Color565::from_rgb(c.r, c.g, c.b);
}

[[nodiscard]] graphics::Color565 terrain_colour(std::uint8_t terrain, std::uint8_t variant) noexcept {
    const std::size_t first = terrain < 9 ? terrain * 2U : 18U + (terrain - 9U);
    const std::size_t index = std::min<std::size_t>(first + variant, kTerrainColours.size() - 1);
    return to565(kTerrainColours[index]);
}

[[nodiscard]] graphics::Color565 scaled(graphics::Color565 c, std::uint32_t n) noexcept {
    const std::uint32_t v  = c.bits();
    const std::uint32_t rb = ((v & 0xF81FU) * n) & (0xF81FU << 4);
    const std::uint32_t g  = ((v & 0x07E0U) * n) & (0x07E0U << 4);
    return graphics::Color565{static_cast<std::uint16_t>((rb + g) >> 4)};
}

[[nodiscard]] graphics::Color565 dimmed(graphics::Color565 c) noexcept {
    const std::uint16_t v = c.bits();
    return graphics::Color565{static_cast<std::uint16_t>(v - ((v >> 2) & 0x39E7U))};
}

struct TileMarks {
    std::int8_t colour = -1;
    bool darkened      = false;
};

[[nodiscard]] std::vector<TileMarks> mark_tiles(const assets::Assets& assets, const model::Map& map, std::int32_t level) {
    const std::int32_t w = map.dimension();
    std::vector<TileMarks> marks(static_cast<std::size_t>(w * w));
    for (const model::MapActor& actor : map.actors()) {
        if (actor.level != level || actor.is_army())
            continue;
        const bool ownable = std::ranges::find(kOwnable, actor.family) != kOwnable.end();
        const bool darkens = actor.family == "decorative";
        if (!ownable && !darkens)
            continue;
        const formats::AdvObject* model = assets.object(actor.model);
        if (model == nullptr)
            continue;
        for (std::int32_t j = 0; j < model->size().height; ++j)
            for (std::int32_t i = 0; i < model->size().width; ++i) {
                if (!model->covers(i, j))
                    continue;
                const std::int32_t x = actor.position.x + i;
                const std::int32_t y = actor.position.y + j;
                if (x < 0 || y < 0 || x >= w || y >= w)
                    continue;
                TileMarks& mark = marks[static_cast<std::size_t>(x + y * w)];
                if (ownable && mark.colour < 0)
                    mark.colour = static_cast<std::int8_t>(std::min<std::uint8_t>(actor.owner, 6));
                if (darkens)
                    mark.darkened = true;
            }
    }
    return marks;
}

void stroke(const graphics::Canvas& canvas, const core::Rect& r, graphics::Color565 colour) {
    const auto fill = [&](std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
        for (std::int32_t y = y0; y < y1; ++y)
            for (std::int32_t x = x0; x < x1; ++x)
                canvas.set_pixel({x, y}, colour);
    };
    if (r.width() < 3 || r.height() < 3) {
        fill(r.left(), r.top(), r.right(), r.bottom());
        return;
    }
    fill(r.left(), r.top(), r.right(), r.top() + 1);
    fill(r.left(), r.bottom() - 1, r.right(), r.bottom());
    fill(r.left(), r.top() + 1, r.left() + 1, r.bottom() - 1);
    fill(r.right() - 1, r.top() + 1, r.right(), r.bottom() - 1);
}

}

MiniMap::MiniMap(const assets::Assets& assets, const model::Map& map, const MapRenderer& renderer, core::Size panel, std::int32_t level)
: image_{panel} {
    const formats::ScenarioGrid& grid  = map.terrain();
    const model::Shroud* shroud        = renderer.shroud_plane();
    const std::vector<TileMarks> marks = mark_tiles(assets, map, level);

    const std::int64_t w = panel.width, h = panel.height, s = map.dimension();
    if (w <= 0 || h <= 0 || s <= 0)
        return;
    for (std::int64_t y = 0; y < h; ++y)
        for (std::int64_t x = 0; x < w; ++x) {
            const auto col = static_cast<std::int32_t>(((y * w - x * h) + w * h) * s / (2 * w * h));
            const auto row = static_cast<std::int32_t>((x * h + y * w) * s / (2 * w * h));
            const core::Point at{static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
            const formats::GridTile* tile = grid.tile(col, row, level);
            if (tile == nullptr)
                continue;

            const auto colour_of = [&] {
                if (tile->misc != 0)
                    return graphics::Color565{};
                const TileMarks& mark = marks[static_cast<std::size_t>(col + row * s)];
                if (mark.colour >= 0)
                    return to565(kPlayerColours[static_cast<std::size_t>(mark.colour)]);
                graphics::Color565 c = terrain_colour(tile->terrain, tile->variant);
                if (mark.darkened)
                    c = dimmed(c);
                const int index = graphics::shade_index(renderer.slope_at(col, row, level) + 0x100, at.x & 1, at.y & 1);
                return graphics::shade(c, static_cast<unsigned>(index));
            };

            graphics::Color565 colour{};
            if (shroud == nullptr)
                colour = colour_of();
            else {
                const std::span<const model::ShroudPiece> record = shroud->record(col, row, level);
                if (shroud->level(col, row, level) == model::kFogHidden) {
                    if (!record.empty())
                        colour = scaled(colour_of(), 5);
                } else {
                    colour = colour_of();
                    if (std::ranges::any_of(record, [](const model::ShroudPiece& piece) {
                            return piece.level == model::kFogHidden;
                        }))
                        colour = scaled(colour, 11);
                }
            }
            image_.set(at, colour);
        }
}

core::Rect MiniMap::tile_rect(std::int32_t col, std::int32_t row, std::int32_t s, core::Size panel) noexcept {
    if (s <= 0)
        return {};
    const std::int64_t w = panel.width, h = panel.height, e = s;
    const std::int64_t across = 2 * (static_cast<std::int64_t>(row) - col);
    const std::int64_t down   = 2 * (static_cast<std::int64_t>(col) + row);
    return core::Rect::from_edges(static_cast<std::int32_t>(((e - 2 + across) * w) / (2 * e)),
                                  static_cast<std::int32_t>(((down - e) * h) / (2 * e)),
                                  static_cast<std::int32_t>(((e + 2 + across) * w + 2 * e - 1) / (2 * e)),
                                  static_cast<std::int32_t>(((down + 4 - e) * h + 2 * e - 1) / (2 * e)));
}

void MiniMap::paint(const graphics::Canvas& canvas, core::Point at, const core::Rect& view, core::Size extent) const {
    canvas.draw_surface(image_, at);
    if (extent.width <= 0 || extent.height <= 0)
        return;
    const core::Point top_left     = to_panel(view.origin(), extent, image_.size());
    const core::Point bottom_right = to_panel({view.right(), view.bottom()}, extent, image_.size());
    const core::Rect outer = core::Rect::from_edges(top_left.x - 1, top_left.y - 1, bottom_right.x + 1, bottom_right.y + 1);
    const graphics::Canvas panel = canvas.child({at, image_.size()});
    stroke(panel, outer, graphics::Color565{});
    stroke(panel,
           core::Rect::from_edges(outer.left() + 1, outer.top() + 1, outer.right() - 1, outer.bottom() - 1),
           graphics::Color565::from_rgb(255, 255, 255));
}

}
