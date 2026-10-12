#include "h4/rules/adventure_board.hpp"

#include <algorithm>
#include <array>

#include "h4/core/ascii.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/scenario/grid.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/model/map.hpp"

namespace h4::rules {

namespace {

struct Footprint {
    const formats::AdvObject* model = nullptr;

    [[nodiscard]] core::Size size() const noexcept {
        return model != nullptr ? model->size() : core::Size{1, 1};
    }
    [[nodiscard]] bool covers(std::int32_t i, std::int32_t j) const noexcept {
        return model != nullptr && model->covers(i, j);
    }
    [[nodiscard]] bool passage_y(std::int32_t i, std::int32_t j) const noexcept {
        return model != nullptr ? model->passage_y(i, j) : i == 0 && (j == 0 || j == 1);
    }
    [[nodiscard]] bool passage_x(std::int32_t i, std::int32_t j) const noexcept {
        return model != nullptr ? model->passage_x(i, j) : j == 0 && (i == 0 || i == 1);
    }
    [[nodiscard]] bool wall_y(std::int32_t i, std::int32_t j, bool bridge) const noexcept {
        return model != nullptr && model->wall_y(i, j, bridge);
    }
    [[nodiscard]] bool wall_x(std::int32_t i, std::int32_t j, bool bridge) const noexcept {
        return model != nullptr && model->wall_x(i, j, bridge);
    }
    [[nodiscard]] bool touches(std::int32_t i, std::int32_t j) const noexcept {
        return model != nullptr ? model->touches(i, j) : i == 0 && j == 0;
    }
};

constexpr std::array<std::uint32_t, 49> kLandCodes{4,    40,   101,  102,  103,  121,  123,  201,  202,  203,
                                                   212,  301,  302,  303,  312,  331,  333,  400,  1010, 1012,
                                                   1030, 1202, 1203, 1210, 1212, 1230, 1233, 2020, 2021, 2030,
                                                   2031, 2101, 2120, 2121, 2301, 2331, 3010, 3012, 3020, 3030,
                                                   3033, 3120, 3123, 3302, 3303, 3312, 3330, 3333, 4000};

[[nodiscard]] bool land_mask(std::uint32_t mask) noexcept {
    for (const std::uint32_t code : kLandCodes)
        if (formats::transition_code_key(code) == mask)
            return true;
    return false;
}

[[nodiscard]] bool is_water(std::uint8_t terrain) noexcept {
    return terrain == 0 || terrain == 9;
}

[[nodiscard]] bool
tile_is_dry(const formats::ScenarioGrid& grid, const formats::GridTile& tile, const formats::TransitionLibrary* transitions) {
    if (!is_water(tile.terrain))
        return true;
    if (transitions == nullptr)
        return false;
    std::uint32_t mask = 0;
    const auto all     = grid.overlays_a();
    for (std::uint32_t i = 0; i < tile.overlay_a_count; ++i) {
        const std::size_t at = static_cast<std::size_t>(tile.overlay_a) + i;
        if (at >= all.size() || is_water(all[at].terrain))
            continue;
        const formats::TransitionSet* set = transitions->slot(all[at].tail);
        if (set == nullptr)
            continue;
        const formats::TransitionEntry* entry = set->entry(all[at].field);
        if (entry == nullptr)
            continue;
        if (const std::uint32_t key = entry->coverage_key(); key < 0x100)
            mask |= key;
    }
    return land_mask(mask);
}

}

AdventureBoard::AdventureBoard(const model::Map& map)
: map_{&map}, tiles_(map.terrain().tiles().size()), lists_(2 * tiles_.size()) {
}

const formats::ScenarioGrid& AdventureBoard::grid() const noexcept {
    return map_->terrain();
}

std::int32_t AdventureBoard::index(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const formats::GridTile* at = grid().tile(x, y, level);
    return at == nullptr ? -1 : static_cast<std::int32_t>(at - grid().tiles().data());
}

MapPoint AdventureBoard::at_index(std::int32_t slot) const noexcept {
    if (slot < 0 || static_cast<std::size_t>(slot) >= where_.size())
        return {};
    const auto& w = where_[static_cast<std::size_t>(slot)];
    return {w[0], w[1], w[2], false};
}

const BoardTile* AdventureBoard::tile(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const std::int32_t at = index(x, y, level);
    return at < 0 ? nullptr : &tiles_[static_cast<std::size_t>(at)];
}

std::span<const std::uint32_t> AdventureBoard::objects(const MapPoint& at) const noexcept {
    const std::int32_t slot = index(at.x, at.y, at.level);
    if (slot < 0)
        return {};
    const Range range = lists_[static_cast<std::size_t>(2 * slot + (at.bridge ? 1 : 0))];
    return std::span<const std::uint32_t>{listed_}.subspan(range.first, range.count);
}

bool AdventureBoard::dry_land(std::int32_t x, std::int32_t y, std::int32_t level) const noexcept {
    const std::int32_t at = index(x, y, level);
    return at >= 0 && dry_[static_cast<std::size_t>(at)];
}

const formats::AdvObject* AdventureBoard::model(std::uint32_t actor) const noexcept {
    return actor < models_.size() ? models_[actor] : nullptr;
}

bool AdventureBoard::is_army(std::uint32_t actor) const noexcept {
    const auto actors = map_->actors();
    return actor < actors.size() && actors[actor].is_army();
}

bool AdventureBoard::owns_steps(std::uint32_t actor) const noexcept {
    const auto actors = map_->actors();
    return actor >= actors.size() || actors[actor].family != "event_trigger";
}

bool AdventureBoard::blocked(const MapPoint& at) const noexcept {
    const BoardTile* here = tile(at.x, at.y, at.level);
    if (here == nullptr)
        return true;
    return at.bridge ? here->bridge == 0 : here->blocked != 0;
}

void AdventureBoard::list(std::uint32_t actor, std::int32_t slot, bool bridge) {
    entries_.emplace_back(2 * slot + (bridge ? 1 : 0), actor);
}

void AdventureBoard::stamp(std::uint32_t actor, const formats::AdvObject* model, const MapPoint& origin, int sign) {
    const auto bump = [sign](std::uint8_t& counter) { counter = static_cast<std::uint8_t>(counter + sign); };
    const Footprint foot{model};
    const bool bridge    = model != nullptr && model->is_bridge();
    const bool span      = model != nullptr && model->is_span();
    const core::Size box = foot.size();
    const auto at        = [&](std::int32_t i, std::int32_t j) -> BoardTile* {
        const std::int32_t slot = index(origin.x + i, origin.y + j, origin.level);
        return slot < 0 ? nullptr : &tiles_[static_cast<std::size_t>(slot)];
    };

    for (std::int32_t i = 0; i < box.width; ++i)
        for (std::int32_t j = 0; j < box.height; ++j) {
            BoardTile* here = at(i, j);
            if (here == nullptr)
                continue;
            if (bridge)
                bump(here->bridge);
            if (span) {
                bump(here->span);
                if (i == 0 && !foot.wall_x(i, j, false))
                    bump(here->ramp_x);
                if (j == 0 && !foot.wall_y(i, j, false))
                    bump(here->ramp_y);
            }
            if (foot.covers(i, j)) {
                bump(here->wall_y[0]);
                bump(here->wall_x[0]);
                if (BoardTile* below = at(i, j + 1); below != nullptr)
                    bump(below->wall_y[0]);
                if (BoardTile* right = at(i + 1, j); right != nullptr)
                    bump(right->wall_x[0]);
                bump(here->blocked);
            }
            for (const int layer : {0, 1}) {
                if (foot.wall_y(i, j, layer == 1))
                    bump(here->wall_y[static_cast<std::size_t>(layer)]);
                if (foot.wall_x(i, j, layer == 1))
                    bump(here->wall_x[static_cast<std::size_t>(layer)]);
            }
            if (sign > 0 && foot.touches(i, j))
                list(actor, index(origin.x + i, origin.y + j, origin.level), origin.bridge);
            if (foot.passage_y(i, j))
                bump(here->passage_y[0]);
            if (foot.passage_x(i, j))
                bump(here->passage_x[0]);
        }
    for (std::int32_t i = 0; i < box.width; ++i)
        if (BoardTile* here = at(i, box.height); here != nullptr) {
            if (foot.passage_y(i, box.height))
                bump(here->passage_y[0]);
            for (const int layer : {0, 1})
                if (foot.wall_y(i, box.height, layer == 1))
                    bump(here->wall_y[static_cast<std::size_t>(layer)]);
            if (span && !foot.wall_y(i, box.height, false))
                bump(here->ramp_y);
        }
    for (std::int32_t j = 0; j < box.height; ++j)
        if (BoardTile* here = at(box.width, j); here != nullptr) {
            if (foot.passage_x(box.width, j))
                bump(here->passage_x[0]);
            for (const int layer : {0, 1})
                if (foot.wall_x(box.width, j, layer == 1))
                    bump(here->wall_x[static_cast<std::size_t>(layer)]);
            if (span && !foot.wall_x(box.width, j, false))
                bump(here->ramp_x);
        }
}

AdventureBoard AdventureBoard::build(const model::Map& map,
                                     std::span<const formats::AdvObject* const> models,
                                     const formats::TransitionLibrary* transitions) {
    AdventureBoard board{map};
    board.models_.assign(models.begin(), models.end());
    board.models_.resize(map.actors().size(), nullptr);
    board.origins_.resize(map.actors().size());
    const auto tiles = map.terrain().tiles();
    board.dry_.resize(tiles.size());
    board.where_.resize(tiles.size());
    const formats::ScenarioGrid& grid = map.terrain();
    for (std::int32_t level = 0; level < grid.levels(); ++level)
        for (std::int32_t x = 0; x < grid.width(); ++x) {
            const formats::GridSpan& span = grid.columns()[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; y < span.stop; ++y)
                if (const std::int32_t at = board.index(x, y, level); at >= 0)
                    board.where_[static_cast<std::size_t>(at)] = {
                        static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(level)
                    };
        }
    for (std::size_t i = 0; i < tiles.size(); ++i)
        board.dry_[i] = tile_is_dry(map.terrain(), tiles[i], transitions);
    const auto actors = map.actors();
    for (std::size_t n = 0; n < actors.size(); ++n) {
        const formats::AdvObject* model = n < models.size() ? models[n] : nullptr;
        if (model == nullptr && !actors[n].model.empty())
            continue;
        const model::MapActor& actor = actors[n];
        board.origins_[n]            = {actor.position.x, actor.position.y, actor.level, false};
        board.stamp(static_cast<std::uint32_t>(n), model, board.origins_[n], +1);
    }
    board.flatten();
    return board;
}

void AdventureBoard::flatten() {
    for (Range& range : lists_)
        range = Range{};
    for (const auto& [slot, actor] : entries_)
        ++lists_[static_cast<std::size_t>(slot)].count;
    std::uint32_t next = 0;
    for (Range& range : lists_) {
        range.first = next;
        next += range.count;
        range.count = 0;
    }
    listed_.resize(next);
    for (const auto& [slot, actor] : entries_) {
        Range& range                         = lists_[static_cast<std::size_t>(slot)];
        listed_[range.first + range.count++] = actor;
    }
}

void AdventureBoard::move_army(std::uint32_t actor, const MapPoint& to) {
    if (!is_army(actor))
        return;
    MapPoint& origin = origins_[actor];
    stamp(actor, nullptr, origin, -1);
    std::erase_if(entries_, [actor](const auto& entry) { return entry.second == actor; });
    origin = to;
    stamp(actor, nullptr, origin, +1);
    flatten();
}

void AdventureBoard::remove(std::uint32_t actor) {
    if (actor >= models_.size() || models_[actor] == nullptr)
        return;
    stamp(actor, models_[actor], origins_[actor], -1);
    models_[actor] = nullptr;
    std::erase_if(entries_, [actor](const auto& entry) { return entry.second == actor; });
    flatten();
}

MapPoint AdventureBoard::origin(std::uint32_t actor) const noexcept {
    return actor < origins_.size() ? origins_[actor] : MapPoint{};
}

}
