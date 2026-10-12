#include "h4/screens/boot/loading_view.hpp"

#include <algorithm>
#include <string_view>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/dialog_frame.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/meter.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kWordFontSize = 18;
constexpr std::int32_t kGap          = 2;
constexpr std::int32_t kWordHeight   = 18;

}

core::Result<std::unique_ptr<LoadingView>> LoadingView::build(const assets::Assets& assets, core::Size frame, LoadingModel model) {
    return compose_view<LoadingView>(assets, frame, model);
}

LoadingView::LoadingView(core::Size frame, LoadingModel model) : ModalPopup{frame}, model_{model} {
}

core::Result<void> LoadingView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"icons.progress_bar", "", "loading"});
    if (!opened)
        return std::unexpected{opened.error()};
    const ui::DialogFrameArt art = opened->frame_art("text_background.small");
    const core::Rect bars        = opened->group().bounds();

    const ui::FrameInsets inset = art.insets();
    const core::Size size{bars.width() + inset.across(), bars.height() + kGap + kWordHeight + inset.down()};
    Builder dialog = opened->centred("loading", HelpDecision::unread("the loading dialog's builder"), size);
    hold_dialog(dialog.parent());
    auto& chrome = dialog.add<ui::DialogFrame>(HelpDecision::from_parent(), core::Rect{{}, size}, art);

    const core::Point at = chrome.client_rect().origin();
    Builder bar          = dialog.panel(core::Rect{at, bars.size()}, HelpDecision::from_parent());
    bar_                 = &bar.parent();
    fill_layer_          = &bar.layer("bar");
    fill_                = &bar.add<ui::Meter>(HelpDecision::from_parent(),
                                               core::Rect{{}, bars.size()},
                                               ui::Meter::Direction::rightwards,
                                               ui::Meter::Art{{fill_layer_}, 0, ui::IconCell::Placement::authored});
    for (const std::string_view edge : {"top", "bottom", "left", "right"})
        bar.sprite(edge, HelpDecision::from_parent());

    dialog.label(core::Rect{{at.x, at.y + bars.height() + kGap}, {bars.width(), kWordHeight}},
                 dialog.text("Loading.dialog"),
                 ui::kEngravedCentred,
                 HelpDecision::from_parent(),
                 kWordFontSize);

    bind(model_);
    return dialog.finish();
}

void LoadingView::bind(const LoadingModel& model) {
    model_.percent = std::clamp(model.percent, 0, 100);
    fill_->set_fraction(model_.percent, 100);
}

core::Rect LoadingView::bar_rect() const noexcept {
    return bar_->rect_in_root();
}

core::Rect LoadingView::filled_rect() const noexcept {
    const core::Rect fill = fill_layer_->bounds();
    return core::Rect{fill.origin(), {fill_->length(), fill.height()}}.translated(fill_->origin_in_root());
}

}
