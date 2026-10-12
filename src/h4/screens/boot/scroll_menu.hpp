#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "h4/core/clock.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/ui/widget.hpp"

namespace h4::formats {
class Font;
class Layer;
}
namespace h4::ui {
class Panel;
class Sprite;
class TwoToneLabel;
}
namespace h4::screens {
class Builder;
}

namespace h4::screens {

class ScrollMenu final : public ui::Widget {
public:
    ScrollMenu(Builder& builder, core::Point top_right, std::span<const std::string> labels);

    void advance(core::Instant now);

    [[nodiscard]] bool unrolled() const noexcept {
        return steps_left_ == 0;
    }
    [[nodiscard]] std::size_t entry_count() const noexcept {
        return banners_.size();
    }
    [[nodiscard]] core::Rect entry_rect(std::size_t index) const noexcept;
    [[nodiscard]] bool entry_washed(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect scroll_rect() const noexcept {
        return rect_in_root();
    }
    [[nodiscard]] core::Rect body_rect() const noexcept;
    [[nodiscard]] core::Rect border_rect() const noexcept;
    [[nodiscard]] core::Rect curl_rect() const noexcept;

    core::Signal<std::size_t> entry_clicked;

private:
    struct Art {
        const formats::Layer* top_bar;
        const formats::Layer* background;
        const formats::Layer* left;
        const formats::Layer* right;
        const formats::Layer* parchment;
        const formats::Layer* bottom_border;
        const formats::Layer* border_size;
        const formats::Layer* text_size;
        std::vector<const formats::Layer*> curls;
        const formats::Font* font;
    };
    [[nodiscard]] static Art load(Builder& builder);

    [[nodiscard]] std::int32_t full_height() const noexcept;
    void add_body();
    void add_banners(std::span<const std::string> labels, const std::vector<std::int32_t>& heights);
    void add_frame();
    void show_step(std::int32_t steps_left);

    Art art_;
    std::int32_t content_ = 0;
    ui::Panel* body_      = nullptr;
    ui::Sprite* border_   = nullptr;
    std::vector<ui::Sprite*> curls_;
    std::vector<ui::TwoToneLabel*> banners_;
    std::int32_t steps_left_ = 0;
    core::Timer unroll_{std::chrono::milliseconds{30}};
    core::Connections connections_;
};

}
