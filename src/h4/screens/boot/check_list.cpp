#include "h4/screens/boot/check_list.hpp"

#include <algorithm>
#include <format>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

CheckList::CheckList(Builder& builder, std::span<const CheckModel> rows)
: Grouping{builder, "each check row carries its own options_menu help"} {
    Builder in                      = builder.inside(*this);
    const ui::ButtonFaces box_faces = in.faces("checkbox", "");
    const std::int32_t face         = box_faces.released().bounds().height();
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const CheckModel& row = rows[i];
        const auto help       = in.help(row.help_item);
        in.label(std::format("{}_text", row.guide), in.text(row.text_key), ui::kEngravedLeft, help);
        const core::Rect box = in.guide(std::format("{}_checkbox", row.guide));
        auto& toggle         = in.toggle(box.origin(), box_faces, help);
        connections_.relay(toggle.toggled, toggled, i);
        checks_.push_back(&toggle);
        rows_bottom_ = std::max(rows_bottom_, box.top() + face);
        box_left_    = box.left();
    }
}

void CheckList::set_checked(std::size_t index, bool checked) {
    if (index < checks_.size())
        checks_[index]->set_checked(checked);
}

bool CheckList::checked(std::size_t index) const noexcept {
    return index < checks_.size() && checks_[index]->checked();
}

core::Rect CheckList::check_rect(std::size_t index) const noexcept {
    return index < checks_.size() ? ui::shown_rect(checks_[index]) : core::Rect{};
}

}
