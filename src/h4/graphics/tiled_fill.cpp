#include "h4/graphics/tiled_fill.hpp"

#include <cstdint>

#include "h4/graphics/canvas.hpp"
#include "h4/graphics/surface565.hpp"

namespace h4::graphics {

namespace {

[[nodiscard]] std::int32_t wrap(std::int32_t a, std::int32_t n) noexcept {
    return a >= 0 ? a % n : n - ((-1 - a) % n) - 1;
}

struct Node {
    std::int32_t left, top, right, bottom;
    core::Point source;
    std::int32_t depth;
};

struct Quadrant {
    bool right;
    bool bottom;
};

struct Variant {
    Quadrant paints;
    Quadrant first;
    Quadrant second;
};

[[nodiscard]] constexpr Variant variant(WedgeCorner corner) noexcept {
    switch (corner) {
    case WedgeCorner::top_left: return {{false, false}, {false, true}, {true, false}};
    case WedgeCorner::top_right: return {{true, false}, {true, true}, {false, false}};
    case WedgeCorner::bottom_left: return {{false, true}, {false, false}, {true, true}};
    case WedgeCorner::bottom_right: break;
    }
    return {{true, true}, {true, false}, {false, true}};
}

[[nodiscard]] Node child(const Node& node, Quadrant quadrant) noexcept {
    const std::int32_t mid_x = node.left + node.right;
    const std::int32_t mid_y = node.top + node.bottom;
    const core::Point step{quadrant.right ? node.right - node.left : 0, quadrant.bottom ? node.bottom - node.top : 0};
    return Node{quadrant.right ? mid_x : 2 * node.left,
                quadrant.bottom ? mid_y : 2 * node.top,
                quadrant.right ? 2 * node.right : mid_x,
                quadrant.bottom ? 2 * node.bottom : mid_y,
                core::Point{2 * node.source.x, 2 * node.source.y} + step,
                node.depth + 1};
}

void descend(const core::Rect& clip, const Node& node, const Variant& shape, const WedgeVisitor& paint) {
    const std::int32_t d = node.depth;
    const core::Rect box = core::Rect::from_edges(node.left >> d, node.top >> d, node.right >> d, node.bottom >> d);
    if (box.empty() || box.intersected(clip).empty())
        return;

    const std::int32_t mid_x  = (node.left + node.right) >> (d + 1);
    const std::int32_t mid_y  = (node.top + node.bottom) >> (d + 1);
    const core::Rect quadrant = core::Rect::from_edges(shape.paints.right ? mid_x : box.left(),
                                                       shape.paints.bottom ? mid_y : box.top(),
                                                       shape.paints.right ? box.right() : mid_x,
                                                       shape.paints.bottom ? box.bottom() : mid_y);
    const core::Rect painted = quadrant.intersected(clip);
    if (!painted.empty())
        paint(painted, core::Point{node.source.x >> d, node.source.y >> d} + (painted.origin() - quadrant.origin()));

    if (box.width() == 1 && box.height() == 1)
        return;
    descend(clip, child(node, shape.first), shape, paint);
    descend(clip, child(node, shape.second), shape, paint);
}

}

Color565 texel_wrapped(const Surface565& texture, core::Point source) noexcept {
    const core::Size size = texture.size();
    if (size.empty())
        return {};
    return texture.at({wrap(source.x, size.width), wrap(source.y, size.height)});
}

void fill_tiled(const Canvas& canvas, const Surface565& texture, const core::Rect& rect, core::Point source) noexcept {
    const core::Rect area = rect.intersected(canvas.local_clip());
    for (std::int32_t y = area.top(); y < area.bottom(); ++y)
        for (std::int32_t x = area.left(); x < area.right(); ++x)
            canvas.set_pixel({x, y}, texel_wrapped(texture, source + (core::Point{x, y} - rect.origin())));
}

void walk_wedge(const core::Rect& rect, core::Point source, WedgeCorner corner, const core::Rect& clip, const WedgeVisitor& paint) {
    descend(clip, Node{rect.left(), rect.top(), rect.right(), rect.bottom(), source, 0}, variant(corner), paint);
}

void fill_wedge(const Canvas& canvas, const Surface565& texture, const core::Rect& rect, core::Point source, WedgeCorner corner) {
    walk_wedge(rect, source, corner, canvas.local_clip(), [&](const core::Rect& quadrant, core::Point from) {
        fill_tiled(canvas, texture, quadrant, from);
    });
}

}
