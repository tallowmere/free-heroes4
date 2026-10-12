#pragma once

#include <cstdint>

#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class Panel;
class Scrollbar;

class ScrollView final : public Widget {
public:
    explicit ScrollView(core::Rect viewport, std::int32_t step);

    [[nodiscard]] Panel& sheet() noexcept {
        return *sheet_;
    }

    void attach(Scrollbar& bar);
    void detach();

    void set_content_height(std::int32_t height);

    void set_position(std::int32_t position);
    [[nodiscard]] std::int32_t position() const noexcept {
        return position_;
    }
    [[nodiscard]] std::int32_t steps() const noexcept {
        return steps_;
    }
    [[nodiscard]] std::int32_t overflow() const noexcept {
        return overflow_;
    }

private:
    void move_sheet();
    void drive_bar();

    Panel* sheet_   = nullptr;
    Scrollbar* bar_ = nullptr;
    core::Connection bar_changed_;
    std::int32_t step_;
    std::int32_t overflow_ = 0;
    std::int32_t steps_    = 0;
    std::int32_t position_ = 0;
};

}
