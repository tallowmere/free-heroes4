#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::ui {
class ToggleButton;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

struct CheckModel {
    std::string_view guide;
    std::string_view text_key;
    std::string_view help_item;
};

class CheckList final : public Grouping {
public:
    CheckList(Builder& builder, std::span<const CheckModel> rows);

    void set_checked(std::size_t index, bool checked);
    [[nodiscard]] bool checked(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect check_rect(std::size_t index) const noexcept;
    [[nodiscard]] std::int32_t rows_bottom() const noexcept {
        return rows_bottom_;
    }
    [[nodiscard]] std::int32_t box_left() const noexcept {
        return box_left_;
    }

    core::Signal<std::size_t, bool> toggled;

private:
    std::vector<ui::ToggleButton*> checks_;
    std::int32_t rows_bottom_ = 0;
    std::int32_t box_left_    = 0;
    core::Connections connections_;
};

}
