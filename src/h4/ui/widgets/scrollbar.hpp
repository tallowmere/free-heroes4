#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "h4/core/signal.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/button.hpp"

namespace h4::ui {

class Sprite;

struct ScrollbarArt {
    [[nodiscard]] static std::optional<ScrollbarArt> from_group(const formats::BitmapGroup& group);

    [[nodiscard]] std::int32_t width() const noexcept;
    [[nodiscard]] std::int32_t height() const noexcept;

    ButtonFaces up;
    ButtonFaces down;
    const formats::Layer* background;
    const formats::Layer* thumb;
};

enum class ScrollbarOrientation : std::uint8_t { vertical, horizontal };

class Scrollbar final : public Widget {
public:
    Scrollbar(core::Point origin,
              std::int32_t length,
              const ScrollbarArt& art,
              std::int32_t min,
              std::int32_t max,
              ScrollbarOrientation orientation = ScrollbarOrientation::vertical);

    core::Signal<std::int32_t> changed;

    void set_range(std::int32_t min, std::int32_t max);
    void set_position(std::int32_t position);

    [[nodiscard]] std::int32_t position() const noexcept {
        return position_;
    }
    [[nodiscard]] std::int32_t minimum() const noexcept {
        return min_;
    }
    [[nodiscard]] std::int32_t maximum() const noexcept {
        return max_;
    }
    [[nodiscard]] std::int32_t page() const noexcept {
        return page_;
    }
    [[nodiscard]] core::Rect track_rect() const noexcept {
        return track_;
    }
    [[nodiscard]] core::Rect thumb_rect() const noexcept;

protected:
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_press(core::Point local) override;
    void on_drag(core::Point local) override;
    void on_release(core::Point local) override;

private:
    [[nodiscard]] bool vertical() const noexcept {
        return orientation_ == ScrollbarOrientation::vertical;
    }
    [[nodiscard]] std::int32_t along(core::Point point) const noexcept;
    [[nodiscard]] std::int32_t start(const core::Rect& rect) const noexcept;
    [[nodiscard]] std::int32_t extent(const core::Rect& rect) const noexcept;
    [[nodiscard]] std::int32_t travel() const noexcept;
    void place_thumb();

    const formats::Layer* thumb_layer_;
    ScrollbarOrientation orientation_;
    Sprite* thumb_ = nullptr;
    core::Rect track_;
    std::int32_t min_      = 0;
    std::int32_t max_      = 0;
    std::int32_t page_     = 0;
    std::int32_t position_ = 0;
    std::optional<std::int32_t> grab_;
    core::Connections connections_;
};

}
