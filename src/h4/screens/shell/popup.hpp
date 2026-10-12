#pragma once

#include "h4/core/geometry.hpp"
#include "h4/screens/shell/view.hpp"

namespace h4::screens {

class Popup : public View {
public:
    [[nodiscard]] virtual bool holds(core::Point point) const = 0;

    [[nodiscard]] virtual bool covers(core::Point point) const = 0;
};

}
