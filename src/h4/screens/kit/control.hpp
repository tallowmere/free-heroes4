#pragma once

#include <string_view>
#include <utility>

#include "h4/core/geometry.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::formats {
class BitmapGroup;
}

namespace h4::screens {

template <class T = ui::Button, class... Args>
T& add_control(ui::Widget& parent, const HelpDecision& help, const ui::ButtonFaces& faces, core::Point marker, Args&&... args) {
    return add_window<T>(parent, help, marker - faces.released().bounds().origin(), faces, std::forward<Args>(args)...);
}

inline ui::Sprite& add_control(ui::Widget& parent, const HelpDecision& help, const formats::Layer& layer, core::Point marker) {
    return add_window<ui::Sprite>(parent, help, layer, marker - layer.bounds().origin());
}

[[nodiscard]] ui::ButtonFaces
with_borrowed_disabled(ui::ButtonFaces faces, const formats::BitmapGroup& group, std::string_view disabled_base);

}
