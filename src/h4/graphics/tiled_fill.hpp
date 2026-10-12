#pragma once

#include <functional>

#include "h4/core/geometry.hpp"
#include "h4/graphics/color565.hpp"

namespace h4::graphics {

class Canvas;
class Surface565;

[[nodiscard]] Color565 texel_wrapped(const Surface565& texture, core::Point source) noexcept;

void fill_tiled(const Canvas& canvas, const Surface565& texture, const core::Rect& rect, core::Point source) noexcept;

enum class WedgeCorner { top_left, top_right, bottom_left, bottom_right };

using WedgeVisitor = std::function<void(const core::Rect&, core::Point)>;

void walk_wedge(const core::Rect& rect, core::Point source, WedgeCorner corner, const core::Rect& clip, const WedgeVisitor& paint);

void fill_wedge(const Canvas& canvas, const Surface565& texture, const core::Rect& rect, core::Point source, WedgeCorner corner);

}
