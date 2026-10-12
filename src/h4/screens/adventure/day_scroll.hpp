#pragma once

#include <string>
#include <string_view>

#include "h4/core/geometry.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {
class Label;
}

namespace h4::screens {

class Builder;

class DayScroll final : public ui::Widget {
public:
    DayScroll(Builder& root, core::Point at);

    void set_day(std::string words);
    [[nodiscard]] std::string_view day_text() const noexcept;

private:
    ui::Label* words_ = nullptr;
};

}
