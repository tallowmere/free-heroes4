#pragma once

#include <array>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::ui {
class Button;
}

namespace h4::screens {

class Builder;

class FrameButtons final : public Grouping {
public:
    FrameButtons(Builder& root, const AdventureLayout& layout);

    [[nodiscard]] core::Rect button_rect(AdventureButton button) const noexcept;

    core::Signal<AdventureButton> clicked;

private:
    std::array<ui::Button*, kAdventureButtons.size()> buttons_{};
    core::Connections connections_;
};

}
