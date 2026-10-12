#include "h4/screens/kit/control.hpp"

#include "h4/formats/bitmap_group.hpp"

namespace h4::screens {

ui::ButtonFaces with_borrowed_disabled(ui::ButtonFaces faces, const formats::BitmapGroup& group, std::string_view disabled_base) {
    if (faces.disabled() != nullptr)
        return faces;
    if (const auto grey = ui::ButtonFaces::from_group(group, disabled_base); grey && grey->disabled() != nullptr)
        return faces.with_disabled(*grey->disabled());
    return faces;
}

}
