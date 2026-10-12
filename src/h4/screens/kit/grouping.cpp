#include "h4/screens/kit/grouping.hpp"

#include "h4/screens/kit/builder.hpp"

namespace h4::screens {

Grouping::Grouping(const Builder& builder, std::string_view why)
: Widget{core::Rect{{}, builder.parent().rect().size()}} {
    declare_silent(ui::Silence::retail, why);
    see_through();
}

}
