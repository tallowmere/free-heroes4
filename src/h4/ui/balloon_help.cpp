#include "h4/ui/balloon_help.hpp"

#include <algorithm>
#include <variant>

#include "h4/formats/font.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/text.hpp"

namespace h4::ui {

namespace {

constexpr std::int32_t kPadding = 8;
constexpr std::int32_t kBelow   = 20;
constexpr graphics::Rgba kFill{255, 255, 216, 255};

}

void BalloonHelp::input(const platform::InputEvent& event) {
    if (const auto at = platform::pointer_position(event))
        pointer_ = *at;
    idle_.restart();
    text_.clear();
    lines_.clear();
}

void BalloonHelp::show(std::string_view text, core::Point pointer, core::Size root) {
    text_  = text;
    lines_ = graphics::wrap_text(*font_, text_, root.width - kPadding);
    if (lines_.empty())
        return;
    const core::Size words = graphics::wrapped_size(*font_, lines_);
    const core::Size size{words.width + kPadding, words.height + kPadding};

    core::Point at{pointer.x, pointer.y - size.height};
    if (pointer.y < size.height)
        at.y = pointer.y + kBelow;
    if (const std::int32_t overrun = at.x + size.width - root.width; overrun > 0)
        at.x -= overrun;
    rect_ = core::Rect{at, size};
}

void BalloonHelp::paint(const graphics::Canvas& canvas) const {
    if (!shown())
        return;
    canvas.fill_rect(rect_, kFill);
    canvas.stroke_rect(rect_, graphics::colors::black);
    canvas.draw_text_lines(*font_,
                           wrapped_.lines(*font_, text_, rect_.width()),
                           rect_,
                           graphics::TextStyle{graphics::colors::black, std::nullopt},
                           {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre});
}

}
