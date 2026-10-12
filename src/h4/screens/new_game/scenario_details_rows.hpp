#pragma once

#include <array>
#include <cstddef>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"
#include "h4/screens/new_game/scenario_select_layout.hpp"

namespace h4::ui {
class ToggleButton;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

class ScenarioDetailsRows final : public Grouping {
public:
    ScenarioDetailsRows(Builder& builder, ScenarioColumnSet shown);

    [[nodiscard]] ScenarioColumnSet choice() const noexcept;
    [[nodiscard]] core::Rect checkbox_rect(ScenarioColumn column) const noexcept;

private:
    std::array<ui::ToggleButton*, ScenarioSelectLayout::kMaxColumns> boxes_{};
};

}
