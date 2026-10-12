#include "h4/screens/new_game/scenario_list.hpp"

#include <string_view>
#include <utility>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/text.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/click_area.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/list_row.hpp"
#include "h4/ui/widgets/list_view.hpp"
#include "h4/ui/widgets/scrollbar.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kTitleFontSize = 40;
constexpr std::int32_t kNameRightTrim = 8;

constexpr std::array<std::string_view, ScenarioSelectLayout::kMaxColumns> kHeaderHelp{"size", "difficulty", "players", "human_players", "allied", "map_type"};

using Rows = ScenarioSelectLayout;

}

ScenarioList::ScenarioList(Builder& builder, const ScenarioSelectLayout& layout)
: Grouping{builder, "0x7B2B30 helps headers and rows alone"}, assets_{&builder.assets()}, layout_{layout} {
    Builder in        = builder.inside(*this);
    difficulty_icons_ = &in.group("difficulty_icons");
    map_type_icons_   = &in.group("icons.map_type");

    add_rows(in);
    for (ui::Sprite*& divider : dividers_)
        divider = &in.add<ui::Sprite>(HelpDecision::from_parent(), *layout_.vertical_line);
    add_headers(in);

    const ui::ScrollbarArt art = in.scrollbar_art("vertical_scroll");
    scrollbar_ = &in.add<ui::Scrollbar>(HelpDecision::silent("0x7B2B30 helps headers, buttons and rows alone"),
                                        layout_.scrollbar.origin(),
                                        layout_.scrollbar.height(),
                                        art,
                                        0,
                                        0);
    list_->attach(*scrollbar_);
    set_columns(ScenarioColumnSet::retail_default());
}

void ScenarioList::add_rows(Builder& in) {
    const core::Rect area = core::Rect::from_edges(
        layout_.line.left(), layout_.slots.front().top(), layout_.line.right(), layout_.slots.back().bottom()
    );
    std::vector<core::Rect> slot_rects;
    for (std::size_t i = 0; i < Rows::kRows; ++i)
        slot_rects.push_back(layout_.row(i).translated(core::Point{} - area.origin()));
    list_ = &in.add<ui::ListView>(
        HelpDecision::words({in.text("map_name"), {}}),
        area,
        std::move(slot_rects),
        layout_.wash(),
        ui::kSelectionWash,
        [this](std::size_t slot, ui::ListRow& row) { build_slot(slot, row); },
        [this](std::size_t slot, std::size_t entry) { fill_slot(slot, entry); }
    );
    list_->set_name("map_list");
    connections_.relay(list_->row_clicked, row_selected);
    connections_.relay(list_->row_double_clicked, row_activated);
}

void ScenarioList::add_headers(Builder& in) {
    const std::string caption = in.text("map_name");
    auto& title               = in.add<ui::ClickArea>(HelpDecision::words({caption, {}}), layout_.title);
    in.inside(title).label(core::Rect{{}, layout_.title.size()}, caption, ui::kEngravedLeft, HelpDecision::from_parent(), kTitleFontSize);
    headers_[0] = &title;
    connections_.relay(title.clicked, sort_requested, ScenarioColumn::name);

    for (std::size_t i = 0; i < kColumnFaces.size(); ++i) {
        const ui::ButtonFaces faces = in.faces(kColumnFaces[i]);
        auto& header      = in.add<ui::Button>(in.help(kHeaderHelp[i]), core::Point{}, faces);
        headers_[i + 1]   = &header;
        header_widths_[i] = faces.released().bounds().width();
        connections_.relay(header.clicked, sort_requested, kDataColumns[i]);
    }
}

void ScenarioList::build_slot(std::size_t slot, ui::ListRow& row) {
    const formats::Font* font = assets_->font_for_size(row.rect().height());
    if (font == nullptr)
        return;
    SlotCells& cells = cells_[slot];
    const auto text_pair = [&](graphics::HorizontalAlign horizontal) {
        const graphics::TextAlignment align{horizontal, graphics::VerticalAlign::centre};
        const auto plain = ui::LabelStyle{graphics::TextStyle::engraved(), align};
        const auto hot   = ui::LabelStyle{graphics::TextStyle{graphics::colors::white, graphics::colors::black}, align};
        return LabelPair{
            &add_window<ui::Label>(row.plain(), HelpDecision::from_parent(), core::Rect{}, std::string{}, *font, plain),
            &add_window<ui::Label>(row.selected(), HelpDecision::from_parent(), core::Rect{}, std::string{}, *font, hot)
        };
    };
    cells.name = text_pair(graphics::HorizontalAlign::left);
    for (std::size_t d = 0; d < Rows::kMaxColumns; ++d) {
        SlotCells::Column& column = cells.columns[d];
        if (kDataColumns[d] == ScenarioColumn::difficulty || kDataColumns[d] == ScenarioColumn::map_type)
            column.icon = &add_window<ui::IconCell>(
                row.overlay(), HelpDecision::from_parent(), core::Rect{}, nullptr, nullptr, ui::IconCell::Placement::at_origin
            );
        else
            column.text = text_pair(graphics::HorizontalAlign::centre);
    }
}

void ScenarioList::fill_slot(std::size_t slot, std::size_t entry) {
    SlotCells& cells           = cells_[slot];
    const ScenarioRow& content = rows_[entry];
    if (cells.name.plain == nullptr)
        return;
    cells.name.set_text(content.name);
    const std::int32_t height = list_->row(slot).rect().height();
    const auto set_icon = [](ui::IconCell* icon, const formats::BitmapGroup& group, const std::string& name, core::Rect cell) {
        const formats::Layer* layer = name.empty() ? nullptr : group.find(name);
        if (layer == nullptr || layer->is_placeholder()) {
            icon->set_layer(nullptr, ui::IconCell::Placement::at_origin);
            return;
        }
        icon->set_rect(layer->bounds().centred_in(cell));
        icon->set_layer(layer, ui::IconCell::Placement::at_origin);
    };
    for (std::size_t k = 0; k < shown_.size(); ++k) {
        const ScenarioColumn column = shown_[k];
        SlotCells::Column& cell     = cells.columns[data_index(column)];
        switch (column) {
        case ScenarioColumn::size: cell.text.set_text(content.size); break;
        case ScenarioColumn::difficulty:
            set_icon(cell.icon, *difficulty_icons_, content.difficulty_icon, cell_rect(k, height));
            break;
        case ScenarioColumn::players: cell.text.set_text(content.players); break;
        case ScenarioColumn::humans: cell.text.set_text(content.humans); break;
        case ScenarioColumn::allies: cell.text.set_text(content.allies); break;
        case ScenarioColumn::map_type:
            set_icon(cell.icon, *map_type_icons_, content.map_type_icon, cell_rect(k, height));
            break;
        case ScenarioColumn::name: break;
        }
    }
}

void ScenarioList::bind(std::span<const ScenarioRow> rows, ScenarioColumnSet columns, std::size_t selected) {
    rows_.assign(rows.begin(), rows.end());
    if (columns != columns_)
        set_columns(columns);
    list_->set_rows(rows_.size(), selected);
}

void ScenarioList::set_columns(ScenarioColumnSet columns) {
    columns_ = columns;
    shown_   = columns.shown();
    place_headers();
    place_cells();
    list_->refill();
}

void ScenarioList::place_headers() {
    for (std::size_t i = 0; i < Rows::kMaxColumns; ++i) {
        headers_[i + 1]->set_visible(false);
        dividers_[i]->set_visible(false);
    }
    const std::size_t count = shown_.size();
    for (std::size_t k = 0; k < count; ++k) {
        const std::size_t index = data_index(shown_[k]);
        ui::Widget& header      = *headers_[index + 1];
        const core::Rect column = layout_.column(k, count);
        header.set_rect(core::Rect{{column.left() + (column.width() - header_widths_[index]) / 2, header.rect().top()},
                                   header.rect().size()});
        header.set_visible(true);
        dividers_[k]->set_rect(layout_.divider(k, count));
        dividers_[k]->set_visible(true);
    }
}

void ScenarioList::place_cells() {
    const std::int32_t height = layout_.row(0).height();
    const std::int32_t origin = layout_.line.left();
    const core::Rect name     = core::Rect::from_edges(
        layout_.slots.front().left() - origin, 0, layout_.name_right(shown_.size()) - origin - kNameRightTrim, height
    );
    std::array<bool, Rows::kMaxColumns> in_use{};
    for (std::size_t k = 0; k < shown_.size(); ++k) {
        const std::size_t d = data_index(shown_[k]);
        in_use[d]           = true;
        for (SlotCells& cells : cells_)
            if (cells.columns[d].text.plain != nullptr)
                cells.columns[d].text.set_rect(cell_rect(k, height));
    }
    for (SlotCells& cells : cells_) {
        if (cells.name.plain == nullptr)
            continue;
        cells.name.set_rect(name);
        for (std::size_t d = 0; d < Rows::kMaxColumns; ++d) {
            SlotCells::Column& column = cells.columns[d];
            if (column.icon != nullptr)
                column.icon->set_visible(in_use[d]);
            else
                column.text.set_visible(in_use[d]);
        }
    }
}

core::Rect ScenarioList::cell_rect(std::size_t k, std::int32_t height) const noexcept {
    const core::Rect column = layout_.column(k, shown_.size());
    return core::Rect{{column.left() - layout_.line.left(), 0}, {column.width(), height}};
}

void ScenarioList::LabelPair::set_text(const std::string& words) {
    plain->set_text(words);
    hot->set_text(words);
}

void ScenarioList::LabelPair::set_rect(core::Rect rect) {
    plain->set_rect(rect);
    hot->set_rect(rect);
}

void ScenarioList::LabelPair::set_visible(bool visible) {
    plain->set_visible(visible);
    hot->set_visible(visible);
}

std::size_t ScenarioList::selected() const noexcept {
    return list_->selected();
}

std::size_t ScenarioList::first_visible() const noexcept {
    return list_->first();
}

core::Rect ScenarioList::slot_rect(std::size_t slot) const noexcept {
    return slot < Rows::kRows ? list_->row(slot).rect_in_root() : core::Rect{};
}

core::Rect ScenarioList::header_rect(ScenarioColumn column) const noexcept {
    return ui::shown_rect(headers_[std::to_underlying(column)]);
}

core::Rect ScenarioList::scrollbar_rect() const noexcept {
    return scrollbar_->rect_in_root();
}

}
