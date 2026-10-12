#include "h4/ui/widgets/list_row.hpp"

#include "h4/ui/widgets/blend_window.hpp"

namespace h4::ui {

ListRow::ListRow(core::Rect rect, core::Rect wash_rect, Wash wash) : Widget{rect} {
    const core::Rect whole{{}, rect.size()};
    plain_   = &emplace_child<Panel>(whole);
    wash_    = &emplace_child<BlendWindow>(wash_rect, wash.colour, wash.strength);
    hot_     = &emplace_child<Panel>(whole);
    overlay_ = &emplace_child<Panel>(whole);
    set_selected(false);
}

void ListRow::clear() {
    plain_->clear_children();
    hot_->clear_children();
    overlay_->clear_children();
}

void ListRow::set_selected(bool selected) {
    selected_ = selected;
    plain_->set_visible(!selected);
    wash_->set_visible(selected);
    hot_->set_visible(selected);
}

void ListRow::on_double_click(core::Point ) {
    cancel_press();
    double_clicked.emit();
}

}
