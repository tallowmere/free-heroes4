#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/new_game/scenario_list.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::ui {
class Button;
class TextWindow;
}

namespace h4::screens {

struct ScenarioSelectModel {
    std::vector<ScenarioRow> rows;
    ScenarioColumnSet columns = ScenarioColumnSet::retail_default();
    std::size_t selected      = 0;
    std::string description;
    bool next_enabled = true;
};

enum class ScenarioButton : std::uint8_t { cancel, details, next };

class ScenarioSelectView final : public screens::ModalPopup {
    friend ViewFactory;
    explicit ScenarioSelectView(core::Size frame, ScenarioSelectModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<ScenarioSelectView>>
    build(const assets::Assets& assets, core::Size frame, ScenarioSelectModel model);

    void bind(const ScenarioSelectModel& model);

    core::Signal<ScenarioButton> button_clicked;
    core::Signal<std::size_t> row_selected;
    core::Signal<std::size_t> row_activated;
    core::Signal<ScenarioColumn> sort_requested;

    [[nodiscard]] core::Rect slot_rect(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect header_rect(ScenarioColumn column) const noexcept;
    [[nodiscard]] core::Rect scrollbar_rect() const noexcept;
    [[nodiscard]] core::Rect button_rect(ScenarioButton button) const noexcept;
    [[nodiscard]] std::size_t row_count() const noexcept;
    [[nodiscard]] std::size_t selected() const noexcept;
    [[nodiscard]] std::size_t first_visible() const noexcept;
    [[nodiscard]] bool next_enabled() const noexcept;
    [[nodiscard]] ScenarioColumnSet columns() const noexcept;

private:
    core::Result<void> compose(const assets::Assets& assets);

    ScenarioSelectModel model_;
    ScenarioList* list_          = nullptr;
    ui::TextWindow* description_ = nullptr;
    std::array<ui::Button*, 3> buttons_{};
    core::Connections connections_;
    std::string description_title_;
};

}
