#include "h4/model/player_alignment.hpp"

#include <bit>

#include "h4/model/random_passes.hpp"
#include "h4/model/roller.hpp"

namespace h4::model {

Alignment drawn_alignment(std::uint8_t allowed, Roller& roller) noexcept {
    const auto mask = static_cast<unsigned>(allowed) & ((1U << kAllAlignments.size()) - 1);
    const int count = std::popcount(mask);
    if (count == 0)
        return kAllAlignments[roller.next_index(kAllAlignments.size())];
    const std::optional<std::size_t> bit = nth_set_bit(mask, roller.next(count));
    return bit.has_value() ? kAllAlignments[*bit] : kAllAlignments.back();
}

}
