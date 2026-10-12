#pragma once

#include <array>
#include <string_view>

#include "h4/model/alignment.hpp"

namespace h4::model {

struct StartingStack {
    std::string_view creature;
    bool ranged = false;
};

[[nodiscard]] const std::array<StartingStack, 2>& starting_stacks(Alignment alignment) noexcept;

}
