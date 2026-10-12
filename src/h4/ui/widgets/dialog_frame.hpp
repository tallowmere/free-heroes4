#pragma once

#include <optional>

#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

struct FrameInsets {
    std::int32_t left = 0, top = 0, right = 0, bottom = 0;

    [[nodiscard]] constexpr std::int32_t across() const noexcept {
        return left + right;
    }
    [[nodiscard]] constexpr std::int32_t down() const noexcept {
        return top + bottom;
    }
};

struct DialogFrameArt {
    [[nodiscard]] static std::optional<DialogFrameArt> from_group(const formats::BitmapGroup& group);

    [[nodiscard]] core::Rect bounds() const noexcept;
    [[nodiscard]] FrameInsets insets() const noexcept;

    const formats::Layer* top_left;
    const formats::Layer* top;
    const formats::Layer* top_right;
    const formats::Layer* left;
    const formats::Layer* right;
    const formats::Layer* bottom_left;
    const formats::Layer* bottom;
    const formats::Layer* bottom_right;
    const formats::Layer* background;
    core::Rect background_rect;
    core::Rect client_area;
};

class DialogFrame final : public Widget {
public:
    DialogFrame(core::Rect rect, const DialogFrameArt& art) noexcept;

    [[nodiscard]] core::Rect client_rect() const noexcept;

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    DialogFrameArt art_;
    core::Point growth_;
};

}
