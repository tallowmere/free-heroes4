#include "h4/model/scenario_setup.hpp"

#include <algorithm>
#include <bit>
#include <utility>

namespace h4::model {

namespace {

constexpr int kColours    = static_cast<int>(kPlayerColours);
constexpr int kAlignments = 6;

std::optional<Alignment> pinned(std::uint8_t mask) noexcept {
    const auto bits = static_cast<unsigned>(mask);
    if (std::popcount(bits) != 1)
        return std::nullopt;
    return static_cast<Alignment>(std::countr_zero(bits));
}

}

ScenarioSetup ScenarioSetup::fill(const ScenarioSummary& scenario, std::string player_name, std::string computer_name) {
    ScenarioSetup setup;
    setup.computer_name_ = std::move(computer_name);
    std::vector<std::size_t> order;
    for (const ScenarioSlot& s : scenario.slots) {
        const auto index = std::to_underlying(s.colour);
        if (index >= kPlayerColours || setup.slots_[index].available)
            continue;
        setup.slots_[index] = Slot{true, s.human_capable, s.alignments, s.team.value_or(kNoTeam)};
        order.push_back(index);
        if (s.human_capable)
            ++setup.human_slots_;
    }

    const auto& slots         = setup.slots_;
    const auto team_has_human = [&](std::size_t team) {
        return std::ranges::any_of(order, [&](std::size_t s) { return slots[s].team == team && slots[s].human_capable; });
    };
    const auto team_first = [&](std::size_t team) {
        std::size_t first = kPlayerColours;
        for (const std::size_t s : order)
            if (slots[s].team == team)
                first = std::min(first, s);
        return first;
    };
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        const Slot& sa = slots[a];
        const Slot& sb = slots[b];
        if (sa.team != sb.team) {
            const bool ha = team_has_human(sa.team);
            if (ha != team_has_human(sb.team))
                return ha;
            return team_first(sa.team) < team_first(sb.team);
        }
        if (sa.human_capable != sb.human_capable)
            return sa.human_capable;
        return a < b;
    });

    std::vector<std::string> pool{std::move(player_name)};
    for (const std::size_t s : order) {
        SetupRow row;
        row.colour    = static_cast<PlayerColour>(s);
        row.alignment = pinned(slots[s].alignments);
        if (slots[s].human_capable && !pool.empty()) {
            row.human = true;
            row.name  = std::move(pool.back());
            pool.pop_back();
        } else {
            row.name = setup.computer_name_;
        }
        setup.rows_.push_back(std::move(row));
        if (std::ranges::find(setup.team_order_, slots[s].team) == setup.team_order_.end())
            setup.team_order_.push_back(slots[s].team);
    }
    setup.pool_ = std::move(pool);
    return setup;
}

const ScenarioSetup::Slot& ScenarioSetup::slot_of(std::size_t row) const noexcept {
    return slots_[std::to_underlying(rows_[row].colour)];
}

bool ScenarioSetup::human_capable(std::size_t row) const noexcept {
    return row < rows_.size() && slot_of(row).human_capable;
}

std::uint8_t ScenarioSetup::allowed_alignments(std::size_t row) const noexcept {
    return row < rows_.size() ? slot_of(row).alignments : std::uint8_t{0};
}

std::size_t ScenarioSetup::team(std::size_t row) const noexcept {
    if (row >= rows_.size())
        return 0;
    const auto it = std::ranges::find(team_order_, slot_of(row).team);
    return it == team_order_.end() ? 0 : static_cast<std::size_t>(it - team_order_.begin()) + 1;
}

bool ScenarioSetup::colour_arrows_enabled(std::size_t row) const noexcept {
    return row < rows_.size() && rows_[row].human && human_slots_ >= 2;
}

bool ScenarioSetup::alignment_arrows_enabled(std::size_t row) const noexcept {
    return row < rows_.size() && std::popcount(static_cast<unsigned>(slot_of(row).alignments)) != 1;
}

ScenarioSetup ScenarioSetup::with_alignment_step(std::size_t row, int delta) const {
    if (row >= rows_.size() || (delta != 1 && delta != -1))
        return *this;
    const auto mask   = static_cast<unsigned>(slot_of(row).alignments);
    const int current = rows_[row].alignment ? std::to_underlying(*rows_[row].alignment) : -1;
    int a             = current + delta;
    for (;;) {
        if (a >= kAlignments) {
            a = -1;
            break;
        }
        if (a < -1)
            a = kAlignments - 1;
        else if (a == -1)
            break;
        if (a == current)
            break;
        if (((mask >> static_cast<unsigned>(a)) & 1U) != 0)
            break;
        a += delta;
    }
    ScenarioSetup out        = *this;
    out.rows_[row].alignment = a < 0 ? std::nullopt : std::optional{static_cast<Alignment>(a)};
    return out;
}

ScenarioSetup ScenarioSetup::with_colour_step(std::size_t row, int delta) const {
    if (row >= rows_.size() || (delta != 1 && delta != -1))
        return *this;
    const int own = std::to_underlying(rows_[row].colour);
    int slot      = own;
    do {
        slot = (slot + delta + kColours) % kColours;
    } while (slot != own && !slots_[static_cast<std::size_t>(slot)].human_capable);
    if (slot == own)
        return *this;

    ScenarioSetup out         = *this;
    const PlayerColour colour = static_cast<PlayerColour>(slot);
    const auto holder         = std::ranges::find(out.rows_, colour, &SetupRow::colour);
    SetupRow& moved           = out.rows_[row];
    if (holder != out.rows_.end()) {
        std::swap(holder->colour, moved.colour);
        std::swap(holder->alignment, moved.alignment);
    } else {
        moved.colour    = colour;
        moved.alignment = pinned(slots_[static_cast<std::size_t>(slot)].alignments);
    }
    return out;
}

void ScenarioSetup::make_human(std::size_t row) {
    rows_[row].human = true;
    if (pool_.empty())
        return;
    rows_[row].name = std::move(pool_.back());
    pool_.pop_back();
}

ScenarioSetup ScenarioSetup::with_human(std::size_t row, bool human) const {
    if (row >= rows_.size() || rows_[row].human == human || !human_capable(row))
        return *this;
    ScenarioSetup out = *this;
    if (human) {
        out.make_human(row);
    } else {
        out.pool_.push_back(out.rows_[row].name);
        out.rows_[row].human = false;
        out.rows_[row].name  = out.computer_name_;
    }
    if (std::ranges::none_of(out.rows_, &SetupRow::human)) {
        for (std::size_t k = 1; k <= out.rows_.size(); ++k) {
            const std::size_t next = (row + k) % out.rows_.size();
            if (out.human_capable(next)) {
                out.make_human(next);
                break;
            }
        }
    }
    return out;
}

ScenarioSetup ScenarioSetup::with_difficulty_step(int delta) const {
    ScenarioSetup out = *this;
    out.difficulty_   = static_cast<MapDifficulty>(
        std::clamp(std::to_underlying(difficulty_) + delta, 0, int{std::to_underlying(MapDifficulty::impossible)})
    );
    return out;
}

ScenarioSetup ScenarioSetup::tutorial(const ScenarioSummary& scenario, bool guards_move) {
    ScenarioSetup out = fill(scenario, "Squire", "Computer Squire");
    for (SetupRow& row : out.rows_) {
        row.human = out.slot_of(static_cast<std::size_t>(&row - out.rows_.data())).human_capable;
        row.name  = row.human ? "Squire" : "Computer Squire";
    }
    out.difficulty_  = MapDifficulty::easy;
    out.guards_move_ = guards_move;
    return out;
}

ScenarioSetup ScenarioSetup::with_guards_move(bool move) const {
    ScenarioSetup out = *this;
    out.guards_move_  = move;
    return out;
}

}
