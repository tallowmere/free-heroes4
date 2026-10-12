#pragma once

#include <cstdint>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/fit_words.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/dialog_frame.hpp"

namespace h4::formats {
class Font;
}

namespace h4::screens {

namespace detail {

inline constexpr std::int32_t kFontSize = 18;
inline constexpr std::int32_t kGap = 10;

struct DialogArt {
    ui::DialogFrameArt box, small, large;
    ui::ButtonFaces check, cross;
    const formats::Layer* banner      = nullptr;
    const formats::Layer* banner_text = nullptr;
    const formats::Font* font         = nullptr;
    const formats::Font* title_font   = nullptr;

    [[nodiscard]] static DialogArt load(Builder& b) {
        const formats::Layer& banner = b.layer("dialog.title", "banner");
        const formats::Layer& text   = b.layer("dialog.title", "text");
        return DialogArt{b.frame_art("dialog.generic"),
                         b.frame_art("text_background.small"),
                         b.frame_art("text_background.large"),
                         b.faces("button.ok", ""),
                         b.faces("button.cancel", ""),
                         &banner,
                         &text,
                         &b.font(kFontSize),
                         &b.font(text.bounds().height() / 2)};
    }
};

struct DialogGeometry {
    core::Size box;
    core::Rect title;
    const ui::DialogFrameArt* scroll_art = nullptr;
    core::Rect scroll;
    Words words;
    std::vector<core::Rect> pictures;
    std::vector<Words> captions;
    std::int32_t column = 0, row_left = 0, row_top = 0;
    std::int32_t picture_height = 0;
    core::Rect buttons;
};

[[nodiscard]] DialogGeometry measure(const DialogArt& art, const BasicDialogContent& content, core::Size root);

}

}
