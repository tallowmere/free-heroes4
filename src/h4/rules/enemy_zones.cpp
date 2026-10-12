#include "h4/rules/enemy_zones.hpp"

#include <array>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <vector>

namespace h4::rules {

namespace {

constexpr std::int32_t kReach = 2;

struct ZoneNode {
    MapPoint at;
    MapPoint from;
    ZoneCell cell;
};

class ZoneQueue {
public:
    [[nodiscard]] bool empty() const noexcept {
        return nodes_.empty();
    }

    void push(const ZoneNode& node) {
        auto at = nodes_.begin();
        for (std::ptrdiff_t count = std::ssize(nodes_); count > 0;) {
            const std::ptrdiff_t half = count / 2;
            if (node.cell.cost < (at + half)->cell.cost) {
                at += half + 1;
                count -= half + 1;
            } else
                count = half;
        }
        nodes_.insert(at, node);
    }

    ZoneNode pop() {
        const ZoneNode out = nodes_.back();
        nodes_.pop_back();
        return out;
    }

private:
    std::vector<ZoneNode> nodes_;
};

[[nodiscard]] std::int32_t manhattan(const MapPoint& a, const MapPoint& b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

}

EnemyZones::EnemyZones(const AdventureBoard& board) : board_{&board}, cells_(2 * board.tile_count()) {
}

EnemyZones EnemyZones::mark(const StepTest& searcher, std::span<const ZoneEnemy> enemies) {
    EnemyZones out{searcher.board()};
    for (const ZoneEnemy& enemy : enemies) {
        const std::int32_t slot = out.board_->slot(enemy.at);
        if (slot < 0)
            continue;
        out.cells_[static_cast<std::size_t>(slot)] = ZoneCell{true, true, 0, 0};
        if (!enemy.walked_past)
            out.walk(searcher, enemy.at);
    }
    return out;
}

ZoneCell EnemyZones::at(const MapPoint& at) const noexcept {
    const std::int32_t slot = board_->slot(at);
    return slot < 0 ? ZoneCell{} : cells_[static_cast<std::size_t>(slot)];
}

bool EnemyZones::offer(std::int32_t slot, const ZoneCell& record) {
    ZoneCell& cell = cells_[static_cast<std::size_t>(slot)];
    if (!zone_takes(cell, record))
        return false;
    cell = record;
    return true;
}

void EnemyZones::walk(const StepTest& searcher, const MapPoint& enemy) {
    const AdventureBoard& board = *board_;
    const bool sea              = board.at_sea(enemy);
    std::array<ZoneCell, (2 * kReach + 1) * (2 * kReach + 1)> local{};

    ZoneQueue open;
    open.push({enemy, enemy, ZoneCell{true, true, 0, 0}});
    while (!open.empty()) {
        const ZoneNode node         = open.pop();
        const std::int32_t distance = manhattan(node.at, enemy);
        for (int dir = 0; dir < kDirections; ++dir) {
            const std::optional<StepTest::Neighbour> next = searcher.neighbour(node.at, dir);
            if (!next.has_value())
                continue;
            const MapPoint& to  = next->to;
            const bool diagonal = (dir & 1) != 0;
            bool open_way = true;
            if (!next->ordinary) {
                open_way = !diagonal_corners_walled(board, node.at, to, dir);
                if (!diagonal && !step_owner(board, node.at, dir, searcher.rules().owns).has_value())
                    continue;
            }
            const std::int32_t dx = to.x - enemy.x;
            const std::int32_t dy = to.y - enemy.y;
            if (std::abs(dx) > kReach || std::abs(dy) > kReach)
                continue;
            if (manhattan(to, enemy) < distance)
                continue;

            ZoneNode record{to,
                            node.at,
                            ZoneCell{true,
                                     open_way,
                                     static_cast<std::int8_t>((dir - 4) & 7),
                                     static_cast<std::int8_t>(node.cell.cost + (diagonal ? 3 : 2))}};
            ZoneCell& mine = local[static_cast<std::size_t>((dx + kReach) * (2 * kReach + 1) + dy + kReach)];
            if (!zone_takes(mine, record.cell))
                continue;
            if (sea ? board.dry_land(to.x, to.y, to.level) : board.at_sea(to))
                continue;

            if (open_way && std::abs(dx) < 2 && std::abs(dy) < 2 && !(node.at == enemy) && node.from == enemy &&
                !diagonal && enemy.bridge == to.bridge) {
                const int d    = record.cell.dir;
                const int came = node.cell.dir;
                if (((d + 2) & 7) == came || ((d - 2) & 7) == came) {
                    record.from      = enemy;
                    record.cell.cost = 3;
                    record.cell.dir  = static_cast<std::int8_t>((((d - 2) & 7) == came ? d - 1 : d + 1) & 7);
                }
            }
            if (!offer(board.slot(to), record.cell))
                continue;
            mine = record.cell;
            if (next->ordinary)
                open.push(record);
        }
    }
}

}
