#pragma once

#include <string_view>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/screens/kit/window_builder.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::ui {
class Button;
class Widget;
}

namespace h4::screens {

[[nodiscard]] core::Result<ui::Button*>
add_large_button(ui::Widget& parent, const assets::Assets& assets, core::Point guide_origin, std::string_view text_key, const HelpDecision& help);

}
