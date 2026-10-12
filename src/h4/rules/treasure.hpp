#pragma once

#include <cstdint>

#include "h4/model/resources.hpp"

namespace h4::model {
class Roller;
}

namespace h4::rules {

[[nodiscard]] model::ResourceBundle roll_pile(model::Resource resource, model::Roller& roller) noexcept;

[[nodiscard]] std::int32_t roll_chest(model::Roller& roller) noexcept;

[[nodiscard]] constexpr std::int32_t chest_experience(std::int32_t gold) noexcept {
    return gold - 500;
}

}
