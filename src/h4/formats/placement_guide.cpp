#include "h4/formats/placement_guide.hpp"

#include <cstdint>

namespace h4::formats {

namespace {

constexpr std::size_t kGuidePen = 4;

bool exact_shape(const Layer& l, std::int32_t w) noexcept {
    const auto rows = l.rows();
    if (rows[0].x0 != 0 || rows[0].x1 != w)
        return false;
    for (std::size_t y = 1; y < rows.size(); ++y)
        if (rows[y].x0 != 0 || rows[y].x1 != 1)
            return false;
    return true;
}

bool sloppy_shape(const Layer& l, std::int32_t w, std::int32_t h) noexcept {
    const auto rows = l.rows();
    if (rows[0].x0 != 0 || rows[0].x1 * 2 < w)
        return false;
    for (const RowSpan& r : rows)
        if (r.x0 != 0 || r.x1 <= r.x0)
            return false;
    const auto ink  = static_cast<std::int64_t>(l.ink());
    const auto area = std::int64_t{w} * h;
    return ink <= 2 * (std::int64_t{w} + h) && ink * 4 < area;
}

bool thick_shape(const Layer& l, std::int32_t w) noexcept {
    const auto rows = l.rows();
    std::size_t top = 0;
    while (top < rows.size() && rows[top].x1 * 2 >= w)
        ++top;
    if (top == 0 || top > kGuidePen || top >= rows.size())
        return false;
    for (std::size_t y = top; y < rows.size(); ++y)
        if (rows[y].x0 >= kGuidePen || rows[y].x1 > kGuidePen || rows[y].x1 <= rows[y].x0)
            return false;
    return true;
}

}

bool is_placement_guide(const Layer& layer) noexcept {
    if (layer.is_placeholder())
        return false;
    const std::int32_t w = layer.bounds().width();
    const std::int32_t h = layer.bounds().height();
    if (w < 2 || h < 2)
        return false;
    return exact_shape(layer, w) || sloppy_shape(layer, w, h) || thick_shape(layer, w);
}

}
