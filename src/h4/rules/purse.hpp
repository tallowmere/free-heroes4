#pragma once

#include "h4/model/resources.hpp"
#include "h4/model/scenario_summary.hpp"

namespace h4::rules {

[[nodiscard]] model::ResourceBundle starting_purse(bool computer, model::MapDifficulty difficulty) noexcept;

}
