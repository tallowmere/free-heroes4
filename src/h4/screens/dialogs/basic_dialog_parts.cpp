#include <optional>

#include "h4/graphics/text.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "h4/screens/dialogs/basic_dialog_detail.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

using detail::kGap;

void BasicDialog::add_title(const detail::DialogArt& art, const detail::DialogGeometry& layout) {
    auto& title = dialog().emplace_child<ui::Panel>(layout.title);
    add_window<ui::Sprite>(title, HelpDecision::from_parent(), *art.banner, core::Point{} - art.banner->bounds().origin());
    add_window<ui::Label>(title,
                          HelpDecision::from_parent(),
                          art.banner_text->bounds().translated(core::Point{} - art.banner->bounds().origin()),
                          content_.title,
                          *art.title_font,
                          ui::LabelStyle{graphics::TextStyle{graphics::colors::black, std::nullopt},
                                         {graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre}});
}

void BasicDialog::add_words(const detail::DialogArt& art, const detail::DialogGeometry& layout) {
    const ui::FrameInsets inset = layout.scroll_art->insets();
    auto& sheet = add_window<ui::DialogFrame>(dialog(), HelpDecision::from_parent(), layout.scroll, *layout.scroll_art);
    sheet.set_name("scroll");
    scroll_rect_ = layout.scroll.translated(dialog().rect().origin());
    add_window<ui::Label>(sheet,
                          HelpDecision::from_parent(),
                          core::Rect{{inset.left, inset.top}, layout.words.size},
                          content_.text,
                          *art.font,
                          ui::LabelStyle{graphics::TextStyle::engraved(),
                                         {content_.centred ? graphics::HorizontalAlign::centre : graphics::HorizontalAlign::left,
                                          graphics::VerticalAlign::top}});
}

void BasicDialog::add_items(const detail::DialogArt& art, const detail::DialogGeometry& layout) {
    for (std::size_t i = 0; i < content_.items.size(); ++i) {
        const DialogItem& item    = content_.items[i];
        const auto left           = layout.row_left + static_cast<std::int32_t>(i) * (layout.column + kGap);
        const core::Rect& picture = layout.pictures[i];
        const core::Point origin{left + (layout.column - picture.width()) / 2,
                                 layout.row_top + layout.picture_height - picture.height()};
        auto& holder = add_window<ui::Panel>(dialog(), HelpDecision::words(item.help), core::Rect{origin, picture.size()});
        for (const DialogItem::Piece& piece : item.pieces)
            if (piece.layer != nullptr)
                add_window<ui::Sprite>(holder, HelpDecision::from_parent(), *piece.layer, piece.at - picture.origin());
        item_rects_.push_back(core::Rect{origin, picture.size()}.translated(dialog().rect().origin()));
        const Words& caption = layout.captions[i];
        if (caption.lines.empty())
            continue;
        add_window<ui::Label>(
            dialog(),
            HelpDecision::from_parent(),
            core::Rect{{left + (layout.column - caption.size.width) / 2, layout.row_top + layout.picture_height + kGap},
                       caption.size},
            item.caption,
            *art.font,
            ui::LabelStyle{graphics::TextStyle::engraved(), {graphics::HorizontalAlign::centre, graphics::VerticalAlign::top}}
        );
    }
}

void BasicDialog::add_buttons(Builder& dialog, const detail::DialogArt& art, const detail::DialogGeometry& layout) {
    if (content_.buttons == DialogButtons::none)
        return;
    const core::Rect row = layout.buttons;
    ok_ = &dialog.control(core::Point{row.left(), row.bottom() - art.check.released().bounds().height()},
                          art.check,
                          dialog.help("ok"));
    connections_.on(ok_->clicked, [this] {
        dismissed.emit();
        answered.emit(true);
    });
    if (content_.buttons != DialogButtons::check_and_cross)
        return;
    const core::Size check = art.check.released().bounds().size();
    no_ = &dialog.control(core::Point{row.left() + check.width + kGap, row.bottom() - art.cross.released().bounds().height()},
                          art.cross,
                          dialog.help("cancel"));
    connections_.relay(no_->clicked, answered, false);
}

}
