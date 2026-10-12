#include "h4/screens/adventure/meters.hpp"

#include <format>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::screens {

std::string_view meter_group(std::int32_t guide_height) noexcept {
    return guide_height <= 43 ? "icons.morale.34" : "icons.morale.44";
}

const formats::Layer* meter_layer(const formats::BitmapGroup* group, Meter meter, int value) {
    if (group == nullptr)
        return nullptr;
    return group->find(std::format("{} {}", value, meter == Meter::luck ? "luck" : "morale"));
}

ui::Help meter_help(const assets::Assets& assets, Meter meter, int value) {
    const std::string_view name = meter == Meter::luck ? "luck" : "morale";
    return {value == 0 ? std::string{assets.text(std::format("neutral_{}.dialog", name))} :
                         std::format("{:+} {}", value, assets.text(std::format("{}.dialog", name))),
            std::string{assets.text(std::format("{}.help", name))}};
}

}
