#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"
#include "h4/ui/widgets/radio_group.hpp"

namespace h4::ui {
class ToggleButton;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

struct ResolutionModel {
    std::string_view guide;
    std::string_view text_guide;
    std::int32_t width;
    std::int32_t height;
    std::string_view help_item;
};

class ResolutionSelector final : public Grouping {
public:
    ResolutionSelector(Builder& builder, std::span<const ResolutionModel> resolutions);

    void select(std::size_t index);
    [[nodiscard]] std::size_t selected() const noexcept;
    [[nodiscard]] core::Rect toggle_rect(std::size_t index) const noexcept;

    core::Signal<std::size_t> selection_changed;

private:
    std::vector<ui::ToggleButton*> toggles_;
    std::unique_ptr<ui::RadioGroup> group_;
    core::Connections connections_;
};

}
