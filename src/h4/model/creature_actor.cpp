#include "h4/model/creature_actor.hpp"

#include <cstdint>

namespace h4::model {

std::string_view creature_actor_keyword(int kind) noexcept {
    if (kind < 0 || kind >= kCreatureKinds)
        return {};
    return creature_kind_keyword(CreatureKindId{static_cast<std::uint8_t>(kind)});
}

std::span<const std::string_view> creature_actor_keywords() noexcept {
    return creature_kind_keywords();
}

}
