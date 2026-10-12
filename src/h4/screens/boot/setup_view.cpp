#include "h4/screens/boot/setup_view.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <utility>

#include "h4/formats/font.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/color.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/box.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/text_button.hpp"

namespace h4::screens {

namespace {

constexpr graphics::Rgba kBackground{24, 28, 40, 255};
constexpr graphics::Rgba kPanel{36, 42, 58, 255};
constexpr graphics::Rgba kEdge{150, 160, 190, 255};
constexpr graphics::Rgba kInk{230, 232, 240, 255};
constexpr graphics::Rgba kDim{160, 166, 186, 255};
constexpr graphics::Rgba kFill{200, 170, 90, 255};
constexpr graphics::Rgba kHover{60, 70, 96, 255};
constexpr graphics::Rgba kPressed{90, 104, 140, 255};

constexpr core::Rect kDropArea{{40, 80}, {560, 190}};
constexpr core::Rect kStatus{{40, 290}, {560, 50}};
constexpr core::Rect kBar{{40, 350}, {560, 14}};
constexpr ui::TextButton::Look kButtonLook{kPanel, kHover, kPressed, kEdge, kInk};
constexpr core::Rect kLeftButton{{40, 410}, {130, 30}};
constexpr core::Rect kMiddleButton{{235, 410}, {170, 30}};
constexpr core::Rect kRightButton{{470, 410}, {130, 30}};

[[nodiscard]] std::int32_t percent_of(std::uint64_t done, std::uint64_t total) noexcept {
    if (total == 0)
        return 0;
    return static_cast<std::int32_t>(std::min<std::uint64_t>(done * 100 / total, 100));
}

[[nodiscard]] ui::LabelStyle centred(graphics::Rgba ink) noexcept {
    return {{ink, std::nullopt}, {graphics::HorizontalAlign::centre, graphics::VerticalAlign::top}};
}

[[nodiscard]] std::string_view prompt_text(SetupPrompt prompt) {
    switch (prompt) {
    case SetupPrompt::drop:
        return "Drop the Heroes of Might and Magic IV installer from GOG "
               "here.\n\n"
               "setup_heroes_of_might_and_magic_4_complete_...exe, with its "
               "-1.bin beside "
               "it.\n\n"
               "A folder with an installed game's Data works too.";
    case SetupPrompt::pick_folder:
        return "Browse to the folder holding the Heroes of Might and Magic IV "
               "installer "
               "from GOG: the .exe and its -1.bin.\n\n"
               "Copy both to On My iPhone or On My iPad, openh4, in the Files "
               "app, then "
               "choose that folder.\n\n"
               "A folder with an installed game's Data works too.";
    }
    return {};
}

}

class SetupView::Bar final : public ui::Widget {
public:
    explicit Bar(core::Rect rect) noexcept : Widget{rect} {
    }
    void set_percent(std::int32_t percent) noexcept {
        percent_ = std::clamp(percent, 0, 100);
    }

protected:
    void draw(const graphics::Canvas& canvas) const override {
        const core::Size size = rect().size();
        canvas.fill_rect({{}, size}, kPanel);
        canvas.fill_rect({{2, 2}, {(size.width - 4) * percent_ / 100, size.height - 4}}, kFill);
        canvas.stroke_rect({{}, size}, kEdge);
    }

private:
    std::int32_t percent_ = 0;
};

SetupView::SetupView(core::Size frame, SetupPrompt prompt, bool offer_sign_in)
: WidgetView{frame}, prompt_{prompt}, offer_sign_in_{offer_sign_in},
  font_{std::make_unique<formats::Font>(formats::Font::builtin())} {
}

SetupView::~SetupView() = default;

core::Result<std::unique_ptr<SetupView>> SetupView::build(SetupPrompt prompt, bool offer_sign_in) {
    return compose_view<SetupView>(kFrame, prompt, offer_sign_in);
}

core::Result<void> SetupView::compose() {
    const SetupPrompt prompt = prompt_;
    const bool offer_sign_in = offer_sign_in_;
    ui::Widget& root         = tree().root();
    const auto own = HelpDecision::silent("the port's own screen");
    add_window<ui::Box>(root, own, core::Rect{{}, kFrame}, kBackground, std::nullopt);
    add_window<ui::Label>(root, own, core::Rect{{40, 40}, {560, 20}}, "openh4 -- first start", *font_, centred(kInk));

    auto& drop = add_window<ui::Box>(root, own, kDropArea, kPanel, kEdge);
    std::string text{prompt_text(prompt)};
    if (offer_sign_in)
        text += "\n\nOr sign in with GOG and the port fetches it.";
    add_window<ui::Label>(drop, own, core::Rect{{20, 30}, {520, 150}}, std::move(text), *font_, centred(kDim));

    status_ = &add_window<ui::Label>(root, own, kStatus, "", *font_, centred(kInk));
    bar_    = &add_window<Bar>(root, own, kBar);

    auto& browse = add_window<ui::TextButton>(root, own, kLeftButton, "Browse...", *font_, kButtonLook);
    auto& quit   = add_window<ui::TextButton>(root, own, kRightButton, "Quit", *font_, kButtonLook);
    auto& cancel = add_window<ui::TextButton>(root, own, kRightButton, "Cancel", *font_, kButtonLook);
    connections_.relay(browse.clicked, browse_clicked);
    connections_.relay(quit.clicked, quit_clicked);
    connections_.relay(cancel.clicked, cancel_clicked);
    browse_ = &browse;
    if (offer_sign_in) {
        auto& sign_in = add_window<ui::TextButton>(root, own, kMiddleButton, "Sign in with GOG", *font_, kButtonLook);
        connections_.relay(sign_in.clicked, sign_in_clicked);
        sign_in_ = &sign_in;
    }
    quit_   = &quit;
    cancel_ = &cancel;
    show_waiting("");
    return {};
}

void SetupView::show_busy(bool busy) {
    bar_->set_visible(busy);
    browse_->set_visible(!busy);
    if (sign_in_ != nullptr)
        sign_in_->set_visible(!busy);
    quit_->set_visible(!busy);
    cancel_->set_visible(busy);
}

void SetupView::show_waiting(std::string message) {
    extracting_  = false;
    downloading_ = false;
    percent_     = 0;
    status_->set_text(std::move(message));
    show_busy(false);
}

void SetupView::show_progress(std::size_t files_done, std::size_t files_total, std::uint64_t bytes_done, std::uint64_t bytes_total) {
    extracting_  = true;
    downloading_ = false;
    percent_     = percent_of(bytes_done, bytes_total);
    status_->set_text(std::format("Extracting: {} of {} files", files_done, files_total));
    bar_->set_percent(percent_);
    show_busy(true);
}

void SetupView::show_download(std::uint64_t bytes_done, std::uint64_t bytes_total) {
    extracting_  = false;
    downloading_ = true;
    percent_     = percent_of(bytes_done, bytes_total);
    status_->set_text(bytes_total == 0 ? std::string{"Signing in to GOG..."} :
                                         std::format("Downloading from GOG: {} of {} MB", bytes_done >> 20, bytes_total >> 20));
    bar_->set_percent(percent_);
    show_busy(true);
}

std::string_view SetupView::status() const noexcept {
    return status_->text();
}
core::Rect SetupView::browse_rect() const noexcept {
    return browse_->rect();
}
core::Rect SetupView::sign_in_rect() const noexcept {
    return sign_in_ != nullptr ? sign_in_->rect() : core::Rect{};
}
core::Rect SetupView::quit_rect() const noexcept {
    return quit_->rect();
}
core::Rect SetupView::cancel_rect() const noexcept {
    return cancel_->rect();
}

}
