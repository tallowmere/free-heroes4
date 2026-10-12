#include <algorithm>
#include <array>
#include <unordered_map>

#include "h4/formats/transition_set.hpp"
#include "h4/model/roller.hpp"
#include "h4/model/shroud.hpp"

namespace h4::model {

namespace {

constexpr std::array<core::Point, 8> kDelta{{{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}};

[[nodiscard]] std::unordered_map<std::uint32_t, std::uint16_t> key_table(const formats::TransitionLibrary& transitions) {
    std::unordered_map<std::uint32_t, std::uint16_t> out;
    const formats::TransitionSet* set = transitions.slot(0);
    if (set == nullptr)
        return out;
    for (std::size_t i = 0; i < set->entries().size(); ++i)
        out.try_emplace(set->entries()[i].coverage_key(), static_cast<std::uint16_t>(i));
    return out;
}

[[nodiscard]] std::uint8_t slot(Roller& rolls, bool above) noexcept {
    return static_cast<std::uint8_t>(rolls.next(5) + (above ? 5 : 0));
}

struct Sweep {
    std::array<std::uint8_t, 3> masks{};
    std::vector<ShroudPiece> diagonals;
    int quadrant = 0;

    void bits(std::uint8_t level, std::uint8_t value) noexcept {
        if (level < masks.size())
            masks[level] |= static_cast<std::uint8_t>(value << (2 * quadrant));
    }
};

}

void Shroud::build_records(const formats::TransitionLibrary& transitions) {
    const std::unordered_map<std::uint32_t, std::uint16_t> keys = key_table(transitions);
    Roller rolls{seed_};

    first_.assign(level_.size() + 1, 0);

    const auto neighbour = [&](std::int32_t x, std::int32_t y, std::int32_t plane, int direction) -> std::uint8_t {
        const core::Point delta       = kDelta[static_cast<std::size_t>(direction & 7)];
        std::int32_t nx               = std::clamp(x + delta.x, 0, width_ - 1);
        const formats::GridSpan& span = spans_[static_cast<std::size_t>(nx)];
        const std::int32_t ny         = std::clamp(y + delta.y, span.start, span.stop - 1);
        return level(nx, ny, plane);
    };

    for (std::int32_t plane = 0; plane < planes_; ++plane)
        for (std::int32_t x = 0; x < width_; ++x) {
            const formats::GridSpan& span = spans_[static_cast<std::size_t>(x)];
            for (std::int32_t y = span.start; y < span.stop; ++y) {
                const std::int32_t at = index(x, y, plane);
                if (at < 0)
                    continue;
                first_[static_cast<std::size_t>(at)] = static_cast<std::uint32_t>(pieces_.size());

                const std::uint8_t me = level_[static_cast<std::size_t>(at)];
                Sweep sweep;
                for (int q = 0; q < 4; ++q) {
                    sweep.quadrant            = q;
                    const int dir             = 1 + 2 * q;
                    const std::uint8_t a      = neighbour(x, y, plane, dir - 1);
                    const std::uint8_t across = neighbour(x, y, plane, dir);
                    const std::uint8_t c      = neighbour(x, y, plane, dir + 1);
                    const auto diagonal       = [&](std::uint8_t lvl) {
                        const auto it = keys.find(0x100U + static_cast<std::uint32_t>(q));
                        if (it == keys.end())
                            return;
                        sweep.diagonals.push_back({lvl, it->second, slot(rolls, lvl > me)});
                    };

                    if (a == me) {
                        if (across == me || c == me)
                            continue;
                        if (me < c)
                            sweep.bits(c, 2);
                        if (across != c && across > c && across > me)
                            diagonal(across);
                    } else if (c == me) {
                        if (across == me || a == me)
                            continue;
                        if (me < a)
                            sweep.bits(a, 1);
                        if (across != a && across > a && across > me)
                            diagonal(across);
                    } else if (across == me) {
                        if (a > me && c > me)
                            continue;
                        if (a == c) {
                            if (me < a)
                                sweep.bits(a, 3);
                        } else {
                            if (me < a)
                                sweep.bits(a, 1);
                            if (me < c)
                                sweep.bits(c, 2);
                        }
                    } else if (a == c) {
                        if (a == across || (a < me && a < across))
                            sweep.bits(a, 3);
                        else {
                            if (me < a)
                                sweep.bits(a, 3);
                            if (across > a && across > me)
                                diagonal(across);
                        }
                    } else {
                        if (me < a)
                            sweep.bits(a, 1);
                        if (me < c)
                            sweep.bits(c, 2);
                        if (across > a && across > c && across > me)
                            diagonal(across);
                    }
                }

                for (std::uint8_t lvl = 0; lvl < sweep.masks.size(); ++lvl) {
                    if (sweep.masks[lvl] == 0)
                        continue;
                    const auto it = keys.find(sweep.masks[lvl]);
                    if (it == keys.end())
                        continue;
                    pieces_.push_back({lvl, it->second, slot(rolls, lvl > me)});
                }
                for (const ShroudPiece& piece : sweep.diagonals)
                    pieces_.push_back(piece);
            }
        }

    first_.back() = static_cast<std::uint32_t>(pieces_.size());
}

}
