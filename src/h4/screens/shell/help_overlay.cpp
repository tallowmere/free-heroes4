#include "h4/screens/shell/help_overlay.hpp"

#include <algorithm>
#include <variant>

#include "h4/assets/assets.hpp"
#include "h4/assets/required.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "h4/screens/shell/view.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kBalloonFont = 17;

BasicDialogContent popup_content(std::string_view text) {
    std::string words{text};
    if (!words.empty() && words.front() >= 'a' && words.front() <= 'z')
        words.front() = static_cast<char>(words.front() - 'a' + 'A');
    return {.text = std::move(words), .centred = true, .buttons = DialogButtons::none};
}

core::Point anchor_of(core::Rect owner, core::Size root) noexcept {
    return {std::clamp(owner.centre().x, 0, root.width), std::clamp(owner.bottom(), 0, root.height)};
}

}

core::Result<HelpOverlay> HelpOverlay::build(const assets::Assets& assets) {
    const auto font = assets::require_font(assets, kBalloonFont);
    if (!font)
        return std::unexpected{font.error().with_context("help")};
    const auto probe = BasicDialog::build(assets, {800, 600}, popup_content("help"));
    if (!probe)
        return std::unexpected{probe.error().with_context("help")};
    return HelpOverlay{assets, **font};
}

HelpOverlay::HelpOverlay(const assets::Assets& assets, const formats::Font& balloon_font)
: assets_{&assets}, balloon_{balloon_font} {
}

HelpOverlay::HelpOverlay(HelpOverlay&&) noexcept            = default;
HelpOverlay& HelpOverlay::operator=(HelpOverlay&&) noexcept = default;
HelpOverlay::~HelpOverlay()                                 = default;

bool HelpOverlay::admit(const platform::InputEvent& event) {
    balloon_.input(event);
    if (popup_ == nullptr)
        return true;
    using namespace platform::events;
    if (const auto* pressed = std::get_if<PointerPressed>(&event)) {
        if (pressed->button == platform::PointerButton::primary)
            return false;
        popup_.reset();
    } else if (const auto* released = std::get_if<PointerReleased>(&event)) {
        if (released->button == platform::PointerButton::primary)
            popup_.reset();
    }
    return true;
}

void HelpOverlay::settle(const platform::InputEvent& event, const View& view, core::Size root) {
    const auto* released = std::get_if<platform::events::PointerReleased>(&event);
    if (released == nullptr || released->button != platform::PointerButton::secondary)
        return;
    const ui::HelpAt at = view.help_at(released->position);
    if (at.take_right_click) {
        at.take_right_click();
        return;
    }
    if (at.right_click.empty())
        return;
    auto dialog = BasicDialog::build(
        *assets_, root, popup_content(at.right_click), DialogPlacement::hanging_from(anchor_of(at.right_click_owner, root))
    );
    if (dialog)
        popup_ = std::move(*dialog);
}

void HelpOverlay::advance(core::Instant now, const View& view, core::Size root) {
    if (popup_ != nullptr)
        return;
    balloon_.advance(now, root, [&view](core::Point at) { return view.help_at(at).balloon; });
}

void HelpOverlay::close() noexcept {
    popup_.reset();
}

void HelpOverlay::paint(const graphics::Canvas& canvas) const {
    if (popup_ != nullptr)
        popup_->paint(canvas);
    balloon_.paint(canvas);
}

}
