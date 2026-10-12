#include "h4/screens/dialogs/basic_dialog.hpp"

#include <algorithm>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/dialogs/basic_dialog_detail.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/dialog_frame.hpp"

namespace h4::screens {

namespace {

core::Rect placed(core::Size size, core::Rect root, const DialogPlacement& placement) {
    using Anchor = DialogPlacement::Anchor;
    if (placement.anchor == Anchor::centred)
        return core::Rect{{}, size}.centred_in(root);
    core::Point at{placement.at.x - size.width / 2,
                   placement.anchor == Anchor::standing_on ? placement.at.y - size.height : placement.at.y};
    at.y = std::max(std::min(at.y, root.bottom() - size.height), root.top());
    at.x = std::max(std::min(at.x, root.right() - size.width), root.left());
    return core::Rect{at, size};
}

}

core::Result<std::unique_ptr<BasicDialog>>
BasicDialog::build(const assets::Assets& assets, core::Size frame, BasicDialogContent content, DialogPlacement placement) {
    return compose_view<BasicDialog>(assets, frame, std::move(content), placement);
}

core::Result<void> BasicDialog::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.generic", "shared", "basic dialog"});
    if (!opened)
        return std::unexpected{opened.error()};
    const detail::DialogArt art = detail::DialogArt::load(*opened);
    if (!opened->ok())
        return opened->finish();
    const core::Rect root          = tree().root().rect();
    const detail::DialogGeometry l = detail::measure(art, content_, root.size());
    const bool has_title           = !content_.title.empty();
    const bool has_words           = !content_.text.empty();

    const core::Rect at = placed(l.box, root, placement_);
    auto& dialog = add_window<ui::Panel>(tree().root(), HelpDecision::unread("t_basic_dialog's frame and words (0x558050)"), at);
    dialog.set_name("basic_dialog");
    hold_dialog(dialog);
    add_window<ui::DialogFrame>(dialog, HelpDecision::from_parent(), core::Rect{{}, at.size()}, art.box);
    if (has_title)
        add_title(art, l);
    if (has_words)
        add_words(art, l);
    add_items(art, l);
    Builder inside = opened->within(dialog, core::Point{});
    add_buttons(inside, art, l);
    return opened->finish();
}

core::Rect BasicDialog::button_rect() const noexcept {
    return ui::shown_rect(ok_);
}

core::Rect BasicDialog::no_rect() const noexcept {
    return ui::shown_rect(no_);
}

core::Rect BasicDialog::item_rect(std::size_t index) const noexcept {
    return index < item_rects_.size() ? item_rects_[index] : core::Rect{};
}

}
