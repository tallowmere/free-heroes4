#include <algorithm>
#include <vector>

#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/screens/dialogs/basic_dialog_detail.hpp"

namespace h4::screens::detail {

namespace {

constexpr std::int32_t kSmallWords = 60;
constexpr std::int32_t kWordsMargin   = 150;
constexpr std::int32_t kWordsHeadroom = 200;
constexpr std::int32_t kCaptionMin = 40;
constexpr std::int32_t kCaptionMax = 160;

core::Rect picture_of(const DialogItem& item) {
    core::Rect out;
    bool first = true;
    for (const DialogItem::Piece& piece : item.pieces) {
        if (piece.layer == nullptr)
            continue;
        const core::Rect r = piece.layer->bounds().translated(piece.at);
        out                = first ? r : out.united(r);
        first              = false;
    }
    return out;
}

core::Size buttons_size(const ui::ButtonFaces& check, const ui::ButtonFaces& cross, DialogButtons buttons) {
    const core::Size a = check.released().bounds().size();
    const core::Size b = cross.released().bounds().size();
    switch (buttons) {
    case DialogButtons::none: return {};
    case DialogButtons::check: return a;
    case DialogButtons::check_and_cross: return {a.width + kGap + b.width, std::max(a.height, b.height)};
    }
    return {};
}

core::Size measure_words(DialogGeometry& l, const DialogArt& art, std::string_view text, core::Size root) {
    l.words                     = lay_out(*art.font, text, 0, root.width - kWordsMargin);
    l.words.size.height         = std::min(l.words.size.height, root.height - kWordsHeadroom);
    l.scroll_art                = l.words.size.height < kSmallWords ? &art.small : &art.large;
    const ui::FrameInsets inset = l.scroll_art->insets();
    return {l.words.size.width + inset.left + inset.right, l.words.size.height + inset.top + inset.bottom};
}

std::int32_t measure_items(DialogGeometry& l, const DialogArt& art, const std::vector<DialogItem>& items) {
    std::int32_t caption_height = 0;
    for (const DialogItem& item : items) {
        l.pictures.push_back(picture_of(item));
        l.captions.push_back(lay_out(*art.font, item.caption, kCaptionMin, kCaptionMax));
        l.column         = std::max({l.column, l.pictures.back().width(), l.captions.back().size.width});
        l.picture_height = std::max(l.picture_height, l.pictures.back().height());
        caption_height   = std::max(caption_height, l.captions.back().size.height);
    }
    return caption_height;
}

}

DialogGeometry measure(const DialogArt& art, const BasicDialogContent& content, core::Size root) {
    DialogGeometry l;
    const core::Size scroll           = measure_words(l, art, content.text, root);
    const std::int32_t caption_height = measure_items(l, art, content.items);
    const auto items                  = static_cast<std::int32_t>(content.items.size());
    const std::int32_t row_width      = items > 0 ? items * l.column + (items - 1) * kGap : 0;
    const core::Size buttons          = buttons_size(art.check, art.cross, content.buttons);
    const bool has_words              = !content.text.empty();
    const bool has_title              = !content.title.empty();
    const core::Rect banner           = art.banner->bounds();

    const ui::FrameInsets inset      = art.box.insets();
    const std::int32_t content_width = std::max(
        {has_title ? banner.width() : 0, has_words ? scroll.width : 0, row_width, buttons.width}
    );
    const bool hugs          = content.buttons == DialogButtons::none;
    const core::Size least   = hugs ? core::Size{} : art.box.bounds().size();
    const std::int32_t width = std::max(least.width, content_width + inset.left + inset.right);
    const std::int32_t inner = width - inset.left - inset.right;
    const auto centre        = [&](std::int32_t w) { return inset.left + (inner - w) / 2; };

    std::int32_t y = inset.top;
    l.title        = core::Rect{{(width - banner.width()) / 2, 0}, banner.size()};
    if (has_title)
        y = std::max(y, l.title.bottom());
    l.scroll = core::Rect{{centre(scroll.width), y}, scroll};
    if (has_words)
        y += scroll.height + kGap;
    l.row_left = centre(row_width);
    l.row_top  = y;
    if (items > 0)
        y += l.picture_height + kGap + caption_height + kGap;
    l.buttons = core::Rect{{centre(buttons.width), y}, buttons};
    y     = hugs && (has_words || items > 0) ? y - kGap : y + buttons.height;
    l.box = {width, std::max(least.height, y + inset.bottom)};
    return l;
}

}
