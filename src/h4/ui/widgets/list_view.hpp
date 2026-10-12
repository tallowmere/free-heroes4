#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/blend_window.hpp"

namespace h4::ui {

class ListRow;
class Scrollbar;

class ListView final : public Widget {
public:
    using Builder = std::function<void(std::size_t slot, ListRow& row)>;
    using Filler = std::function<void(std::size_t slot, std::size_t entry)>;

    ListView(core::Rect area, std::vector<core::Rect> slot_rects, core::Rect wash_rect, Wash wash, Builder builder, Filler filler);

    core::Signal<std::size_t> row_clicked;
    core::Signal<std::size_t> row_double_clicked;

    void attach(Scrollbar& bar);

    void set_rows(std::size_t count, std::size_t selected);
    void refill();

    [[nodiscard]] std::size_t first() const noexcept {
        return first_;
    }
    [[nodiscard]] std::size_t count() const noexcept {
        return count_;
    }
    [[nodiscard]] std::size_t selected() const noexcept {
        return selected_;
    }
    [[nodiscard]] std::size_t slots() const noexcept {
        return rows_.size();
    }
    [[nodiscard]] ListRow& row(std::size_t slot) noexcept {
        return *rows_[slot];
    }

private:
    void slot_clicked(std::size_t slot);
    void slot_double_clicked(std::size_t slot);
    void select(std::size_t entry);
    void wash();
    void scroll_to(std::size_t first);

    std::vector<ListRow*> rows_;
    Filler filler_;
    Scrollbar* bar_ = nullptr;
    core::Connection bar_changed_;
    core::Connections row_connections_;
    std::size_t count_    = 0;
    std::size_t selected_ = 0;
    std::size_t first_    = 0;
};

}
