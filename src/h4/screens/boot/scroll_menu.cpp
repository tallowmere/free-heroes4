#include "h4/screens/boot/scroll_menu.hpp"

#include <algorithm>
#include <format>

#include "h4/core/ascii.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/graphics/text.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/sprite.hpp"
#include "h4/ui/widgets/tiled_sprite.hpp"
#include "h4/ui/widgets/two_tone_label.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kBannerFontSize = 20;
constexpr std::int32_t kBannerPadding  = 12;
constexpr std::int32_t kUnrollStep     = 40;

core::Rect shifted(const formats::Layer& layer, std::int32_t dy) {
    return layer.bounds().translated(core::Point{0, dy});
}

std::vector<std::int32_t> banner_heights(const formats::Font& font, std::span<const std::string> labels, std::int32_t width) {
    std::vector<std::int32_t> out;
    for (const std::string& label : labels) {
        const std::int32_t words = graphics::wrapped_size(font, graphics::wrap_text(font, label, width)).height;
        out.push_back(std::max(words, font.point_size()) + kBannerPadding);
    }
    return out;
}

}

ScrollMenu::Art ScrollMenu::load(Builder& builder) {
    Art art{&builder.layer("top_bar"),
            &builder.layer("background"),
            &builder.layer("left"),
            &builder.layer("right"),
            &builder.layer("parchment"),
            &builder.layer("bottom_border"),
            &builder.layer("bottom_border_size"),
            &builder.layer("text_size"),
            {},
            &builder.font(kBannerFontSize)};
    for (const formats::Layer& layer : builder.group().layers())
        if (core::istarts_with(layer.name(), "frame"))
            art.curls.push_back(&layer);
    if (art.curls.empty())
        builder.fail(core::Error{core::ErrorCode::corrupt, "layers.dialog.scroll_menu: no frame curls"});
    return art;
}

ScrollMenu::ScrollMenu(Builder& builder, core::Point top_right, std::span<const std::string> labels)
: Widget{core::Rect{top_right, {}}}, art_{load(builder)} {
    declare_silent(ui::Silence::retail, "the scroll menus make no help call");
    if (art_.curls.empty())
        return;
    const std::vector<std::int32_t> heights = banner_heights(*art_.font, labels, art_.text_size->bounds().width());
    for (const std::int32_t height : heights)
        content_ += height;
    add_body();
    add_banners(labels, heights);
    add_frame();
    set_rect(core::Rect{{top_right.x - art_.top_bar->bounds().width(), top_right.y}, {}});
    const auto count = static_cast<std::int32_t>(art_.curls.size());
    steps_left_      = std::min(count - 1, (full_height() + kUnrollStep - 1) / kUnrollStep);
    show_step(steps_left_);
}

std::int32_t ScrollMenu::full_height() const noexcept {
    return content_ + art_.border_size->bounds().height();
}

void ScrollMenu::add_body() {
    const std::int32_t width = art_.right->bounds().right();
    const std::int32_t top   = art_.background->bounds().top();
    const std::int32_t full  = full_height();
    body_                    = &emplace_child<ui::Panel>(core::Rect{{}, {width, full}});
    body_->set_name("body");
    const core::Point group_origin{0, -top};
    const auto piece = HelpDecision::from_parent();
    add_window<ui::Sprite>(*body_, piece, *art_.parchment, core::Point{0, full - art_.parchment->bounds().bottom()});
    add_window<ui::TiledSprite>(*body_, piece, *art_.background, core::Rect{{}, {width, content_}}, group_origin);
    add_window<ui::TiledSprite>(*body_, piece, *art_.left, core::Rect{{}, {width, full}}, group_origin);
    add_window<ui::TiledSprite>(*body_, piece, *art_.right, core::Rect{{}, {width, full}}, group_origin);
}

void ScrollMenu::add_banners(std::span<const std::string> labels, const std::vector<std::int32_t>& heights) {
    const graphics::TextAlignment align{graphics::HorizontalAlign::left, graphics::VerticalAlign::centre};
    const ui::LabelStyle ink{graphics::TextStyle{graphics::colors::black, std::nullopt}, align};
    const ui::LabelStyle hot{graphics::TextStyle{graphics::colors::white, std::nullopt}, align};
    const core::Rect guide = art_.text_size->bounds();
    std::int32_t y         = 0;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        auto& banner = add_window<ui::TwoToneLabel>(*body_,
                                                    HelpDecision::from_parent(),
                                                    core::Rect{{guide.left(), y}, {guide.width(), heights[i]}},
                                                    labels[i],
                                                    *art_.font,
                                                    ink,
                                                    hot,
                                                    ui::kSelectionWash,
                                                    ui::TwoToneLabel::Mode::hover);
        banner.set_name(std::format("entry_{}", i));
        connections_.relay(banner.clicked, entry_clicked, i);
        banners_.push_back(&banner);
        y += heights[i];
    }
}

void ScrollMenu::add_frame() {
    const auto piece = HelpDecision::from_parent();
    border_          = &add_window<ui::Sprite>(*this, piece, *art_.bottom_border);
    for (const formats::Layer* curl : art_.curls)
        curls_.push_back(&add_window<ui::Sprite>(*this, piece, *curl));
    add_window<ui::Sprite>(*this, piece, *art_.top_bar);
}

void ScrollMenu::show_step(std::int32_t steps_left) {
    const std::int32_t full    = full_height();
    const std::int32_t visible = std::max(0, full - steps_left * kUnrollStep);
    const auto count           = static_cast<std::int32_t>(art_.curls.size());
    const std::int32_t index   = std::clamp(count - (full - visible + kUnrollStep - 1) / kUnrollStep - 1, 0, count - 1);
    const std::int32_t top     = art_.background->bounds().top();

    const std::int32_t curl_shift   = top + visible - art_.background->bounds().bottom();
    const std::int32_t border_shift = curl_shift - art_.border_size->bounds().height();

    body_->set_rect(core::Rect{{0, top}, {body_->rect().width(), std::clamp(visible, 0, full)}});
    border_->set_rect(shifted(*art_.bottom_border, border_shift));
    for (std::size_t i = 0; i < curls_.size(); ++i) {
        const bool shown = static_cast<std::int32_t>(i) == index;
        curls_[i]->set_visible(shown);
        if (shown)
            curls_[i]->set_rect(shifted(*art_.curls[i], curl_shift));
    }

    const std::int32_t width  = art_.top_bar->bounds().width();
    const std::int32_t height = std::max(art_.top_bar->bounds().bottom(),
                                         art_.curls[static_cast<std::size_t>(index)]->bounds().bottom() + curl_shift);
    set_rect(core::Rect{rect().origin(), {width, height}});
}

void ScrollMenu::advance(core::Instant now) {
    if (steps_left_ == 0)
        return;
    while (steps_left_ > 0 && unroll_.take(now))
        --steps_left_;
    show_step(steps_left_);
}

core::Rect ScrollMenu::entry_rect(std::size_t index) const noexcept {
    return index < banners_.size() ? banners_[index]->rect_in_root() : core::Rect{};
}

bool ScrollMenu::entry_washed(std::size_t index) const noexcept {
    return index < banners_.size() && banners_[index]->washed();
}

core::Rect ScrollMenu::body_rect() const noexcept {
    return body_ != nullptr ? body_->rect_in_root() : core::Rect{};
}

core::Rect ScrollMenu::border_rect() const noexcept {
    return border_ != nullptr ? border_->rect_in_root() : core::Rect{};
}

core::Rect ScrollMenu::curl_rect() const noexcept {
    for (const ui::Sprite* curl : curls_)
        if (curl->visible())
            return curl->rect_in_root();
    return {};
}

}
