#pragma once

#include <cstdint>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/graphics/diamond.hpp"

namespace h4::formats {
class TransitionEntry;
}

namespace h4::graphics {

[[nodiscard]] std::uint8_t coverage_blend(std::uint8_t base, std::uint8_t on, std::uint8_t nibble) noexcept;

class CoverageSurface {
public:
    explicit CoverageSurface(core::Size size);

    [[nodiscard]] core::Size size() const noexcept {
        return size_;
    }
    [[nodiscard]] core::Rect bounds() const noexcept {
        return {{}, size_};
    }

    void fill(std::uint8_t value) noexcept;
    [[nodiscard]] std::uint8_t at(core::Point p) const noexcept;
    void set(core::Point p, std::uint8_t value) noexcept;

private:
    core::Size size_;
    std::vector<std::uint8_t> bytes_;
};

void fill_diamond_coverage(CoverageSurface& surface, core::Point anchor, CornerLifts lifts, std::uint8_t value) noexcept;

void blend_diamond_coverage(CoverageSurface& surface,
                            core::Point anchor,
                            CornerLifts lifts,
                            const formats::TransitionEntry& mask,
                            std::uint8_t on,
                            const std::uint8_t* base) noexcept;

}
