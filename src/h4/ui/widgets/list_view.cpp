#include "h4/ui/widgets/list_view.hpp"

#include <algorithm>

#include "h4/ui/widgets/list_row.hpp"
#include "h4/ui/widgets/scrollbar.hpp"

namespace h4::ui {

ListView::ListView(core::Rect area, std::vector<core::Rect> slot_rects, core::Rect wash_rect, Wash wash, Builder builder, Filler filler)
: Widget{area}, filler_{std::move(filler)} {
    for (std::size_t slot = 0; slot < slot_rects.size(); ++slot) {
        auto& row = emplace_child<ListRow>(slot_rects[slot], wash_rect, wash);
        builder(slot, row);
        row_connections_.on(row.clicked, [this, slot] { slot_clicked(slot); });
        row_connections_.on(row.double_clicked, [this, slot] { slot_double_clicked(slot); });
        rows_.push_back(&row);
    }
}

void ListView::attach(Scrollbar& bar) {
    bar_ = &bar;
    bar_changed_.disconnect();
    bar_changed_ = bar_->changed.connect([this](std::int32_t position) { scroll_to(static_cast<std::size_t>(position)); });
    bar_->set_range(0, static_cast<std::int32_t>(count_ > rows_.size() ? count_ - rows_.size() : 0));
    bar_->set_position(static_cast<std::int32_t>(first_));
}

void ListView::set_rows(std::size_t count, std::size_t selected) {
    count_    = count;
    selected_ = count != 0 ? std::min(selected, count - 1) : 0;
    first_ = selected_ >= rows_.size() ? selected_ + 1 - rows_.size() : 0;
    if (bar_ != nullptr) {
        const auto last = static_cast<std::int32_t>(count_ > rows_.size() ? count_ - rows_.size() : 0);
        bar_->set_range(0, last);
        bar_->set_position(static_cast<std::int32_t>(first_));
    }
    refill();
}

void ListView::refill() {
    for (std::size_t slot = 0; slot < rows_.size(); ++slot) {
        const std::size_t entry = first_ + slot;
        rows_[slot]->set_visible(entry < count_);
        if (entry >= count_)
            continue;
        filler_(slot, entry);
        rows_[slot]->set_selected(entry == selected_);
    }
}

void ListView::slot_clicked(std::size_t slot) {
    const std::size_t entry = first_ + slot;
    if (entry >= count_ || entry == selected_)
        return;
    select(entry);
    row_clicked.emit(entry);
}

void ListView::slot_double_clicked(std::size_t slot) {
    const std::size_t entry = first_ + slot;
    if (entry >= count_)
        return;
    if (entry != selected_) {
        select(entry);
        row_clicked.emit(entry);
    }
    row_double_clicked.emit(entry);
}

void ListView::select(std::size_t entry) {
    selected_ = entry;
    wash();
}

void ListView::wash() {
    for (std::size_t slot = 0; slot < rows_.size(); ++slot)
        rows_[slot]->set_selected(first_ + slot == selected_);
}

void ListView::scroll_to(std::size_t first) {
    const std::size_t last = count_ > rows_.size() ? count_ - rows_.size() : 0;
    first                  = std::min(first, last);
    if (first == first_)
        return;
    first_ = first;
    refill();
}

}
