#include "h4/screens/adventure/frame_buttons.hpp"

#include <utility>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"

namespace h4::screens {

FrameButtons::FrameButtons(Builder& root, const AdventureLayout& layout)
: Grouping{root, "each button carries its adventure_map row"} {
    Builder in = root.inside(*this);
    for (const AdventureButton button : kAdventureButtons) {
        const std::size_t index = std::to_underlying(button);
        const core::Rect guide  = layout.buttons[index];
        if (guide.empty())
            continue;
        auto& widget    = in.button(button_spec(button), guide.origin());
        buttons_[index] = &widget;
        connections_.relay(widget.clicked, clicked, button);
    }
    if (ui::Button* underground = buttons_[std::to_underlying(AdventureButton::underground)])
        underground->set_visible(false);
}

core::Rect FrameButtons::button_rect(AdventureButton button) const noexcept {
    const ui::Button* widget = buttons_[std::to_underlying(button)];
    return widget != nullptr ? widget->rect_in_root() : core::Rect{};
}

}
