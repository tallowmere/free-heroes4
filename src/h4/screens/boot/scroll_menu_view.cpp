#include "h4/screens/boot/scroll_menu_view.hpp"

#include "h4/assets/assets.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"

namespace h4::screens {

core::Result<std::unique_ptr<ScrollMenuView>>
ScrollMenuView::build(const assets::Assets& assets, core::Size frame, ScrollMenuModel model) {
    return compose_view<ScrollMenuView>(assets, frame, std::move(model));
}

ScrollMenuView::ScrollMenuView(core::Size frame, ScrollMenuModel model) : WidgetView{frame}, model_{std::move(model)} {
}

core::Result<void> ScrollMenuView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(this->tree().root(), assets, {"dialog.scroll_menu", "", "scroll menu"});
    if (!opened)
        return std::unexpected{opened.error()};
    Builder& builder = *opened;

    menu_ = &builder.add<ScrollMenu>(HelpDecision::from_parent(), builder, model_.anchor, model_.labels);
    connections_.relay(menu_->entry_clicked, chosen);

    return builder.finish();
}

std::size_t ScrollMenuView::entry_count() const noexcept {
    return menu_->entry_count();
}

core::Rect ScrollMenuView::entry_rect(std::size_t index) const noexcept {
    return menu_->entry_rect(index);
}

bool ScrollMenuView::entry_washed(std::size_t index) const noexcept {
    return menu_->entry_washed(index);
}

bool ScrollMenuView::unrolled() const noexcept {
    return menu_->unrolled();
}

core::Rect ScrollMenuView::scroll_rect() const noexcept {
    return menu_->scroll_rect();
}

core::Rect ScrollMenuView::body_rect() const noexcept {
    return menu_->body_rect();
}

core::Rect ScrollMenuView::border_rect() const noexcept {
    return menu_->border_rect();
}

core::Rect ScrollMenuView::curl_rect() const noexcept {
    return menu_->curl_rect();
}

bool ScrollMenuView::holds(core::Point point) const {
    return body_rect().contains(point);
}

bool ScrollMenuView::covers(core::Point point) const {
    return scroll_rect().contains(point);
}

void ScrollMenuView::advance(core::Instant now) {
    menu_->advance(now);
}

}
