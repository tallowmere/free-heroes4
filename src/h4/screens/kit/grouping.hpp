#pragma once

#include <string_view>

#include "h4/ui/widget.hpp"

namespace h4::screens {

class Builder;

class Grouping : public ui::Widget {
protected:
    Grouping(const Builder& builder, std::string_view why);
};

}
