#include "h4/rules/adventure_path.hpp"

#include <algorithm>
#include <utility>

#include "h4/rules/enemy_zones.hpp"

namespace h4::rules {

namespace {

struct OpenEntry {
    std::int32_t key  = 0;
    std::int32_t slot = 0;
};

class OpenSet {
public:
    [[nodiscard]] bool empty() const noexcept {
        return heap_.empty();
    }

    void push(OpenEntry entry) {
        heap_.push_back(entry);
        sift_up(heap_.size() - 1, 0, entry);
    }

    OpenEntry pop() {
        const OpenEntry top  = heap_.front();
        const OpenEntry last = heap_.back();
        heap_.pop_back();
        if (heap_.empty())
            return top;
        std::size_t hole         = 0;
        const std::size_t bottom = heap_.size();
        std::size_t child        = 2;
        for (; child < bottom; child = 2 * child + 2) {
            if (heap_[child - 1].key < heap_[child].key)
                --child;
            heap_[hole] = heap_[child];
            hole        = child;
        }
        if (child == bottom) {
            heap_[hole] = heap_[bottom - 1];
            hole        = bottom - 1;
        }
        sift_up(hole, 0, last);
        return top;
    }

private:
    void sift_up(std::size_t hole, std::size_t top, OpenEntry value) {
        while (top < hole) {
            const std::size_t parent = (hole - 1) / 2;
            if (heap_[parent].key <= value.key)
                break;
            heap_[hole] = heap_[parent];
            hole        = parent;
        }
        heap_[hole] = value;
    }

    std::vector<OpenEntry> heap_;
};

}

PathField::PathField(const AdventureBoard& board, const MapPoint& source)
: board_{&board}, source_{source}, total_(2 * board.tile_count(), -1), from_(2 * board.tile_count(), -1),
  dir_(2 * board.tile_count(), -1) {
}

std::int32_t PathField::slot(const MapPoint& at) const noexcept {
    return board_->slot(at);
}

MapPoint PathField::point(std::int32_t at) const noexcept {
    return board_->at_slot(at);
}

std::optional<std::int32_t> PathField::total(const MapPoint& to) const noexcept {
    const std::int32_t at = slot(to);
    if (at < 0 || to == source_)
        return std::nullopt;
    const std::int32_t cost = total_[static_cast<std::size_t>(at)];
    return cost < 0 ? std::nullopt : std::optional<std::int32_t>{cost};
}

std::optional<PathField::Step> PathField::last_step(const MapPoint& to) const noexcept {
    const std::int32_t at = slot(to);
    if (at < 0 || to == source_ || total_[static_cast<std::size_t>(at)] < 0)
        return std::nullopt;
    return Step{point(from_[static_cast<std::size_t>(at)]), dir_[static_cast<std::size_t>(at)]};
}

std::vector<PathField::PathStep> PathField::path_to(const MapPoint& to) const {
    std::vector<PathStep> out;
    std::int32_t at = slot(to);
    if (at < 0 || to == source_)
        return out;
    while (at >= 0 && total_[static_cast<std::size_t>(at)] > 0) {
        out.push_back({point(at), dir_[static_cast<std::size_t>(at)], total_[static_cast<std::size_t>(at)]});
        at = from_[static_cast<std::size_t>(at)];
    }
    if (at < 0 || !(point(at) == source_))
        return {};
    std::ranges::reverse(out);
    return out;
}

std::vector<MapPoint> PathField::reached() const {
    std::vector<MapPoint> out;
    for (std::size_t at = 0; at < total_.size(); ++at)
        if (total_[at] >= 0)
            out.push_back(point(static_cast<std::int32_t>(at)));
    return out;
}

PathField PathField::search(const AdventureBoard& board, const MapPoint& source, const TerrainCosts& terrain, const Rules& rules) {
    PathField field{board, source};
    const StepTest test{board, rules};
    std::vector<bool> held(field.total_.size(), false);
    std::vector<std::int32_t> steps(field.total_.size(), 0);
    OpenSet open;
    const std::int32_t start = field.slot(source);
    if (start < 0)
        return field;
    field.total_[static_cast<std::size_t>(start)] = 0;
    open.push({0, start});

    while (!open.empty()) {
        const auto [total, at] = open.pop();
        if (total > field.total_[static_cast<std::size_t>(at)])
            continue;
        const MapPoint here = field.point(at);
        if (!(here == source) &&
            (test.blocked(here) || board.at_sea(here) || held[static_cast<std::size_t>(at)] || total > rules.bound))
            continue;
        int only = -1;
        if (rules.zones != nullptr && !(here == source))
            if (const ZoneCell zone = rules.zones->at(here); zone.marked && zone.restricted)
                only = zone.dir;

        for (int dir = 0; dir < kDirections; ++dir) {
            if (only >= 0 && dir != only)
                continue;
            const std::optional<StepTest::Neighbour> next = test.neighbour(here, dir);
            if (!next.has_value() || next->to == source)
                continue;
            const MapPoint& to = next->to;
            const bool water   = board.at_sea(to);
            bool stuck         = false;
            if (!next->ordinary || water) {
                if (step_owner(board, here, dir, rules.owns).has_value()) {
                    if (!next->shut && diagonal_corners_walled(board, here, to, dir))
                        continue;
                } else if (rules.record_walled)
                    stuck = true;
                else
                    continue;
            }
            if (rules.ground.has_value() && !rules.ground->allows(here, to, steps[static_cast<std::size_t>(at)] + 1))
                continue;
            const std::int32_t next_total = total + step_cost(board, here, to, dir, terrain);
            const std::int32_t there      = field.slot(to);
            std::int32_t& best            = field.total_[static_cast<std::size_t>(there)];
            if (best >= 0 && next_total >= best)
                continue;
            best                                         = next_total;
            held[static_cast<std::size_t>(there)]        = stuck;
            steps[static_cast<std::size_t>(there)]       = steps[static_cast<std::size_t>(at)] + 1;
            field.from_[static_cast<std::size_t>(there)] = at;
            field.dir_[static_cast<std::size_t>(there)]  = static_cast<std::int8_t>(dir);
            open.push({next_total, there});
        }
    }
    return field;
}

}
