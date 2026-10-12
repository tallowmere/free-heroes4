#pragma once

#include <array>
#include <memory>

#include "h4/core/error.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/new_game/scenario_details_rows.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::ui {
class Button;
}

namespace h4::screens {

struct ScenarioDetailsModel {
    ScenarioColumnSet shown;
};

enum class DetailsButton : std::uint8_t { cancel, ok };

class ScenarioDetailsView final : public screens::ModalPopup {
    friend ViewFactory;
    explicit ScenarioDetailsView(core::Size frame, ScenarioDetailsModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<ScenarioDetailsView>>
    build(const assets::Assets& assets, core::Size frame, ScenarioDetailsModel model);

    core::Signal<> cancelled;
    core::Signal<ScenarioColumnSet> accepted;

    [[nodiscard]] ScenarioColumnSet choice() const noexcept;
    [[nodiscard]] core::Rect checkbox_rect(ScenarioColumn column) const noexcept;
    [[nodiscard]] core::Rect button_rect(DetailsButton button) const noexcept;

private:
    core::Result<void> compose(const assets::Assets& assets);

    ScenarioDetailsModel model_;
    ScenarioDetailsRows* rows_ = nullptr;
    std::array<ui::Button*, 2> buttons_{};
    core::Connections connections_;
};

}
