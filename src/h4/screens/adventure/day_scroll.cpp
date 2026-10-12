#include "h4/screens/adventure/day_scroll.hpp"

#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/placement_guide.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

[[nodiscard]] core::Rect scroll_rect(Builder& root, core::Point at) {
    return core::Rect{at, root.group("adventure.day_scroll").bounds().size()};
}

}

DayScroll::DayScroll(Builder& root, core::Point at) : Widget{scroll_rect(root, at)} {
    set_name("day_scroll");
    Builder scroll = root.over("adventure.day_scroll", *this);
    for (const formats::Layer& layer : scroll.group().layers())
        if (!layer.is_placeholder() && !formats::is_placement_guide(layer))
            scroll.sprite(layer, HelpDecision::from_parent());
    const core::Rect text = scroll.guide("text");
    words_ = &scroll.label(text, {}, ui::kEngravedCentred, HelpDecision::from_parent(), text.height() / 2);
}

void DayScroll::set_day(std::string words) {
    words_->set_text(std::move(words));
}

std::string_view DayScroll::day_text() const noexcept {
    return words_->text();
}

}
