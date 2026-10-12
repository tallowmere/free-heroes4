#include "h4/screens/boot/resolution_selector.hpp"

#include <format>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

ResolutionSelector::ResolutionSelector(Builder& builder, std::span<const ResolutionModel> resolutions)
: Grouping{builder, "each resolution row carries its own options_menu help"} {
    Builder rows                    = builder.inside(*this);
    const ui::ButtonFaces box_faces = rows.faces("checkbox", "");
    for (const ResolutionModel& r : resolutions) {
        const auto help = rows.help(r.help_item);
        rows.label(r.text_guide, std::format("{} x {}", r.width, r.height), ui::kEngravedLeft, help);
        toggles_.push_back(&rows.toggle(std::format("{}_checkbox", r.guide), box_faces, help));
    }
    group_ = std::make_unique<ui::RadioGroup>(toggles_);
    connections_.relay(group_->selection_changed, selection_changed);
}

void ResolutionSelector::select(std::size_t index) {
    group_->select(index);
}

std::size_t ResolutionSelector::selected() const noexcept {
    return group_->selected();
}

core::Rect ResolutionSelector::toggle_rect(std::size_t index) const noexcept {
    return index < toggles_.size() ? ui::shown_rect(toggles_[index]) : core::Rect{};
}

}
