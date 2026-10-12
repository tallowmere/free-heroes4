#include "h4/rules/step_test.hpp"

namespace h4::rules {

bool GuardGround::allows(const MapPoint& from, const MapPoint& to, std::int32_t steps) const noexcept {
    if (by_steps)
        return to.level == centre.level && steps <= reach;
    const std::int32_t last = 2 * reach;
    const auto rim          = [&](const MapPoint& at) {
        const std::int32_t x = at.x - centre.x + reach;
        const std::int32_t y = at.y - centre.y + reach;
        return x == 0 || y == 0 || x == last || y == last;
    };
    const std::int32_t x = to.x - centre.x + reach;
    const std::int32_t y = to.y - centre.y + reach;
    if (x < 0 || y < 0 || x > last || y > last)
        return false;
    return !rim(to) || !rim(from);
}

bool StepTest::blocked(const MapPoint& at) const {
    if (board_->blocked(at))
        return true;
    if (!rules_->blocks)
        return false;
    for (const std::uint32_t actor : board_->objects(at))
        if (rules_->blocks(actor))
            return true;
    return false;
}

bool StepTest::hidden(const MapPoint& at) const {
    return rules_->hidden && rules_->hidden(at);
}

std::optional<StepTest::Neighbour> StepTest::neighbour(const MapPoint& from, int dir) const {
    const MapPoint to = step_to(*board_, from, dir);
    if (board_->tile(to.x, to.y, to.level) == nullptr)
        return std::nullopt;
    Neighbour out{to, false, blocked(to)};
    out.ordinary = !out.shut && !hidden(to) && !step_walled(*board_, from, dir);
    return out;
}

}
