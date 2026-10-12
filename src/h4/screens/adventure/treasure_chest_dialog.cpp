#include "h4/screens/adventure/treasure_chest_dialog.hpp"

#include <array>
#include <format>
#include <utility>
#include <vector>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/radio_group.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kWordsSize   = 20;
constexpr std::int32_t kCaptionSize = 14;

constexpr ui::LabelStyle kEngravedTop{graphics::TextStyle::engraved(),
                                      {graphics::HorizontalAlign::centre, graphics::VerticalAlign::top}};

}

TreasureChestDialog::~TreasureChestDialog() = default;

core::Result<std::unique_ptr<TreasureChestDialog>>
TreasureChestDialog::build(const assets::Assets& assets, core::Size frame, TreasureChestContent content) {
    return compose_view<TreasureChestDialog>(assets, frame, std::move(content));
}

core::Result<void> TreasureChestDialog::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.treasure_chest", "shared", "treasure chest"});
    if (!opened)
        return std::unexpected{opened.error()};
    const formats::Layer& background = opened->layer("background");
    const ui::ButtonFaces frame      = opened->faces("button.skill_frame", "");
    const core::Point icon           = opened->layer("button.skill_frame", "icon").bounds().origin();

    Builder dialog = opened->centred("treasure_chest",
                                     HelpDecision::unread("the treasure chest dialog's art"),
                                     background.bounds().size());
    hold_dialog(dialog.parent());
    dialog.sprite_at(core::Point{}, background, HelpDecision::from_parent());
    dialog.label("title", content_.title, ui::kEngravedCentred, HelpDecision::from_parent(), kWordsSize);
    dialog.label("dialog_text", content_.text, kEngravedTop, HelpDecision::from_parent(), kWordsSize);

    add_choices(dialog, frame, icon);

    ok_ = &dialog.control("ok_button", dialog.faces("button.ok", ""), dialog.help("ok"));
    connections_.on(ok_->clicked, [this] { chosen.emit(choice()); });
    return dialog.finish();
}

void TreasureChestDialog::add_choices(Builder& dialog, const ui::ButtonFaces& frame, core::Point icon) {
    struct Side {
        std::string_view name;
        const std::string* label;
        std::string_view help_window;
    };
    const std::array<Side, 2> sides{{{"gold", &content_.gold_label, "material_display"},
                                     {"experience", &content_.experience_label, "shared"}}};
    std::vector<ui::ToggleButton*> choices;
    for (const Side& side : sides) {
        auto& choice = dialog.add<ui::ToggleButton>(dialog.help(side.help_window, side.name),
                                                    dialog.marker(std::format("{}_icon", side.name)),
                                                    frame);
        choice.set_inset(dialog.faces(side.name), icon);
        choice_rects_[choices.size()] = choice.rect().translated(dialog_rect().origin());
        choices.push_back(&choice);
        dialog.label(std::format("{}_label", side.name), *side.label, kEngravedTop, HelpDecision::from_parent(), kCaptionSize);
    }
    choices_ = std::make_unique<ui::RadioGroup>(std::move(choices));
    choices_->select(std::to_underlying(Choice::gold));
}

TreasureChestDialog::Choice TreasureChestDialog::choice() const noexcept {
    return static_cast<Choice>(choices_->selected());
}

void TreasureChestDialog::choose(Choice choice) {
    choices_->select(std::to_underlying(choice));
}

core::Rect TreasureChestDialog::button_rect() const noexcept {
    return ok_->rect_in_root();
}

core::Rect TreasureChestDialog::choice_rect(Choice choice) const noexcept {
    return choice_rects_[std::to_underlying(choice)];
}

}
