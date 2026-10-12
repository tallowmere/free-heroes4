#include "h4/rules/treasure.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "h4/model/roller.hpp"

namespace h4::rules {

model::ResourceBundle roll_pile(model::Resource resource, model::Roller& roller) noexcept {
    std::int32_t amount = 0;
    switch (resource) {
    case model::Resource::gold: amount = (roller.next(5) + 4) * 100; break;
    case model::Resource::wood:
    case model::Resource::ore: amount = roller.next(5) + 4; break;
    default: amount = roller.next(3) + 2; break;
    }
    return model::ResourceBundle{}.with(resource, model::ResourceAmount{amount});
}

std::int32_t roll_chest(model::Roller& roller) noexcept {
    const std::int32_t roll = roller.next(10);
    if (roll < 3)
        return 1000;
    if (roll < 6)
        return 1500;
    if (roll < 9)
        return 2000;
    return 0;
}

}
