#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/new_game/player_row_list.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
}
namespace h4::ui {
class Button;
class IconCell;
class Label;
class TextWindow;
class ToggleButton;
}

namespace h4::screens {

struct ScenarioOptionsModel {
    std::vector<PlayerRowModel> rows;
    std::string difficulty_icon;
    std::string difficulty_name;
    bool easier      = false;
    bool harder      = false;
    bool guards_move = false;
    std::string guards_caption;
    std::string conditions;
    std::string description;
};

enum class OptionsButton : std::uint8_t { back, begin };
enum class OptionsArrow : std::uint8_t { up, down };

class ScenarioOptionsView final : public screens::ModalPopup {
    friend ViewFactory;
    explicit ScenarioOptionsView(core::Size frame, ScenarioOptionsModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<ScenarioOptionsView>>
    build(const assets::Assets& assets, core::Size frame, ScenarioOptionsModel model);

    void bind(const ScenarioOptionsModel& model);

    core::Signal<OptionsButton> button_clicked;
    core::Signal<std::size_t, int> alignment_stepped;
    core::Signal<std::size_t, int> colour_stepped;
    core::Signal<std::size_t, bool> human_toggled;
    core::Signal<int> difficulty_stepped;
    core::Signal<bool> guards_toggled;

    [[nodiscard]] core::Rect button_rect(OptionsButton button) const noexcept;
    [[nodiscard]] core::Rect checkbox_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect row_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect alignment_arrow_rect(std::size_t row, OptionsArrow arrow) const noexcept;
    [[nodiscard]] core::Rect colour_arrow_rect(std::size_t row, OptionsArrow arrow) const noexcept;
    [[nodiscard]] core::Rect difficulty_arrow_rect(OptionsArrow arrow) const noexcept;
    [[nodiscard]] core::Rect guards_rect() const noexcept;
    [[nodiscard]] std::string_view difficulty_text() const noexcept;
    [[nodiscard]] std::string_view guards_text() const noexcept;
    [[nodiscard]] core::Rect conditions_scrollbar_rect() const noexcept;
    [[nodiscard]] core::Rect description_scrollbar_rect() const noexcept;

private:
    core::Result<void> compose(const assets::Assets& assets);
    void add_buttons(Builder& dialog);
    void add_difficulty(Builder& dialog);
    void add_prose(Builder& dialog);
    void add_guards(Builder& dialog);
    void add_rows(Builder& dialog);

    ScenarioOptionsModel model_;
    const assets::Assets* assets_                 = nullptr;
    const formats::BitmapGroup* difficulty_icons_ = nullptr;
    PlayerRowList* rows_                          = nullptr;
    ui::IconCell* difficulty_icon_                = nullptr;
    ui::Label* difficulty_text_                   = nullptr;
    ui::Button* easier_                           = nullptr;
    ui::Button* harder_                           = nullptr;
    ui::ToggleButton* guards_                     = nullptr;
    ui::Label* guards_text_                       = nullptr;
    ui::TextWindow* conditions_                   = nullptr;
    ui::TextWindow* description_                  = nullptr;
    std::array<ui::Button*, 2> buttons_{};
    core::Connections connections_;
};

}
