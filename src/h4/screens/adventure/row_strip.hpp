#pragma once

#include <cstddef>
#include <optional>
#include <ranges>

#include "h4/ui/widget.hpp"

namespace h4::screens {

template <std::ranges::range Rows, class Row = std::ranges::range_value_t<Rows>>
void select_row(Rows& rows, std::optional<std::size_t> selected, ui::Widget* Row::* plain, ui::Widget* Row::* highlight) {
    std::size_t i = 0;
    for (Row& row : rows) {
        (row.*plain)->set_visible(i != selected);
        (row.*highlight)->set_visible(i == selected);
        ++i;
    }
}

}
