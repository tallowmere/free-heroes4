#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::ui {
class Label;
class Scrollbar;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

struct SliderModel {
    std::string_view guide;
    std::string_view text_guide;
    std::string_view text_key;
    std::string_view help_item;
    std::int32_t maximum;
};

class SliderGroup final : public Grouping {
public:
    SliderGroup(Builder& builder, std::span<const SliderModel> sliders);

    void set_position(std::size_t index, std::int32_t position);
    void set_value(std::size_t index, std::string value);

    [[nodiscard]] core::Rect slider_rect(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect thumb_rect(std::size_t index) const noexcept;
    [[nodiscard]] std::string_view value(std::size_t index) const noexcept;

    core::Signal<std::size_t, std::int32_t> slider_moved;

private:
    std::vector<ui::Scrollbar*> sliders_;
    std::vector<ui::Label*> values_;
    core::Connections connections_;
};

}
