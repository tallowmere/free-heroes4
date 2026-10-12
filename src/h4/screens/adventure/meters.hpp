#pragma once

#include <cstdint>
#include <string_view>

#include "h4/ui/help.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
class Layer;
}

namespace h4::screens {

enum class Meter : std::uint8_t { luck, morale };

[[nodiscard]] std::string_view meter_group(std::int32_t guide_height) noexcept;

[[nodiscard]] const formats::Layer* meter_layer(const formats::BitmapGroup* group, Meter meter, int value);

[[nodiscard]] ui::Help meter_help(const assets::Assets& assets, Meter meter, int value);

}
