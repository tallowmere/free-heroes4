#pragma once

#include <cstddef>
#include <span>

namespace h4::root {

[[nodiscard]] std::span<const std::byte> app_icon_png() noexcept;

}
