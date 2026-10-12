#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"
#include "h4/screens/new_game/scenario_select_layout.hpp"

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
class ListRow;
class ListView;
class Scrollbar;
class Sprite;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

struct ScenarioRow {
    std::string name;
    std::string size;
    std::string players;
    std::string humans;
    std::string allies;
    std::string difficulty_icon;
    std::string map_type_icon;
};

class ScenarioList final : public Grouping {
public:
    ScenarioList(Builder& builder, const ScenarioSelectLayout& layout);

    void bind(std::span<const ScenarioRow> rows, ScenarioColumnSet columns, std::size_t selected);

    [[nodiscard]] std::size_t row_count() const noexcept {
        return rows_.size();
    }
    [[nodiscard]] std::size_t selected() const noexcept;
    [[nodiscard]] std::size_t first_visible() const noexcept;
    [[nodiscard]] core::Rect slot_rect(std::size_t slot) const noexcept;
    [[nodiscard]] core::Rect header_rect(ScenarioColumn column) const noexcept;
    [[nodiscard]] core::Rect scrollbar_rect() const noexcept;

    core::Signal<std::size_t> row_selected;
    core::Signal<std::size_t> row_activated;
    core::Signal<ScenarioColumn> sort_requested;

private:
    struct LabelPair {
        ui::Label* plain = nullptr;
        ui::Label* hot   = nullptr;

        void set_text(const std::string& words);
        void set_rect(core::Rect rect);
        void set_visible(bool visible);
    };
    struct SlotCells {
        struct Column {
            LabelPair text;
            ui::IconCell* icon = nullptr;
        };
        LabelPair name;
        std::array<Column, ScenarioSelectLayout::kMaxColumns> columns;
    };

    void add_rows(Builder& in);
    void add_headers(Builder& in);
    void build_slot(std::size_t slot, ui::ListRow& row);
    void fill_slot(std::size_t slot, std::size_t entry);
    void set_columns(ScenarioColumnSet columns);
    void place_headers();
    void place_cells();
    [[nodiscard]] core::Rect cell_rect(std::size_t k, std::int32_t height) const noexcept;
    const assets::Assets* assets_;
    ScenarioSelectLayout layout_;
    const formats::BitmapGroup* difficulty_icons_ = nullptr;
    const formats::BitmapGroup* map_type_icons_   = nullptr;

    ui::ListView* list_       = nullptr;
    ui::Scrollbar* scrollbar_ = nullptr;
    std::array<ui::Widget*, ScenarioSelectLayout::kMaxColumns + 1> headers_{};
    std::array<std::int32_t, ScenarioSelectLayout::kMaxColumns> header_widths_{};
    std::array<ui::Sprite*, ScenarioSelectLayout::kMaxColumns> dividers_{};
    std::array<SlotCells, ScenarioSelectLayout::kRows> cells_{};

    std::vector<ScenarioRow> rows_;
    ScenarioColumnSet columns_;
    std::vector<ScenarioColumn> shown_;
    core::Connections connections_;
};

}
