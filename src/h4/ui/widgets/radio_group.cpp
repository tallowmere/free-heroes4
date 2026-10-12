#include "h4/ui/widgets/radio_group.hpp"

#include <algorithm>

#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::ui {

RadioGroup::RadioGroup(std::vector<ToggleButton*> members, bool momentary)
: members_{std::move(members)}, momentary_{momentary} {
    for (std::size_t i = 0; i < members_.size(); ++i)
        if (members_[i]->checked())
            selected_ = i;
    for (std::size_t i = 0; i < members_.size(); ++i)
        connections_.on(members_[i]->toggled, [this, i](bool) {
            if (momentary_) {
                members_[i]->set_checked(i == selected_);
            } else {
                selected_ = i;
                for (std::size_t j = 0; j < members_.size(); ++j)
                    members_[j]->set_checked(j == selected_);
            }
            selection_changed.emit(i);
        });
}

void RadioGroup::select(std::size_t index) {
    selected_ = std::min(index, members_.size() - 1);
    for (std::size_t j = 0; j < members_.size(); ++j)
        members_[j]->set_checked(j == selected_);
}

}
