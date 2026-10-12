#include "h4/ui/widgets/scroll_view.hpp"

#include <algorithm>

#include "h4/ui/widgets/scrollbar.hpp"

namespace h4::ui {

ScrollView::ScrollView(core::Rect viewport, std::int32_t step) : Widget{viewport}, step_{std::max(1, step)} {
    sheet_ = &emplace_child<Panel>(core::Rect{{}, viewport.size()});
}

void ScrollView::attach(Scrollbar& bar) {
    bar_ = &bar;
    drive_bar();
    bar_changed_.disconnect();
    bar_changed_ = bar_->changed.connect([this](std::int32_t position) {
        position_ = std::clamp(position, 0, steps_);
        move_sheet();
    });
}

void ScrollView::detach() {
    bar_ = nullptr;
    bar_changed_.disconnect();
}

void ScrollView::set_content_height(std::int32_t height) {
    overflow_ = std::max(0, height - rect().height());
    steps_    = (overflow_ + step_ - 1) / step_;
    position_ = std::min(position_, steps_);
    sheet_->set_rect(core::Rect{sheet_->rect().origin(), {sheet_->rect().width(), std::max(height, rect().height())}});
    move_sheet();
    drive_bar();
}

void ScrollView::set_position(std::int32_t position) {
    position_ = std::clamp(position, 0, steps_);
    move_sheet();
    if (bar_ != nullptr)
        bar_->set_position(position_);
}

void ScrollView::move_sheet() {
    const std::int32_t offset = std::min(position_ * step_, overflow_);
    sheet_->set_rect(core::Rect{{0, -offset}, sheet_->rect().size()});
}

void ScrollView::drive_bar() {
    if (bar_ == nullptr)
        return;
    bar_->set_range(0, steps_);
    bar_->set_position(position_);
    bar_->set_visible(steps_ > 0);
}

}
