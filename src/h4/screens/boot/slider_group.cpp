#include "h4/screens/boot/slider_group.hpp"

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/scrollbar.hpp"

namespace h4::screens {

SliderGroup::SliderGroup(Builder& builder, std::span<const SliderModel> sliders)
: Grouping{builder, "each slider carries its own options_menu help"} {
    Builder in                 = builder.inside(*this);
    const ui::ScrollbarArt art = in.scrollbar_art("horizontal_scroll");
    for (std::size_t i = 0; i < sliders.size(); ++i) {
        const SliderModel& s  = sliders[i];
        const core::Rect box  = in.guide(s.text_guide);
        const core::Rect rail = in.guide(s.guide);
        const auto help       = in.help(s.help_item);

        in.label(box, in.text(s.text_key), ui::kEngravedLeft, help);
        values_.push_back(&in.label(box, std::string{}, ui::kEngravedRight, help));
        auto& bar = in.add<ui::Scrollbar>(help, rail.origin(), rail.width(), art, 0, s.maximum, ui::ScrollbarOrientation::horizontal);
        connections_.relay(bar.changed, slider_moved, i);
        sliders_.push_back(&bar);
    }
}

void SliderGroup::set_position(std::size_t index, std::int32_t position) {
    if (index < sliders_.size())
        sliders_[index]->set_position(position);
}

void SliderGroup::set_value(std::size_t index, std::string value) {
    if (index < values_.size())
        values_[index]->set_text(std::move(value));
}

core::Rect SliderGroup::slider_rect(std::size_t index) const noexcept {
    return index < sliders_.size() ? ui::shown_rect(sliders_[index]) : core::Rect{};
}

core::Rect SliderGroup::thumb_rect(std::size_t index) const noexcept {
    if (index >= sliders_.size())
        return {};
    const ui::Scrollbar& bar = *sliders_[index];
    return bar.thumb_rect().translated(bar.origin_in_root());
}

std::string_view SliderGroup::value(std::size_t index) const noexcept {
    return index < values_.size() ? values_[index]->text() : std::string_view{};
}

}
