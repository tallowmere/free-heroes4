#include "h4/screens/adventure/army_window_view.hpp"

#include <utility>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/adventure/creature_portrait.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/sprite.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kHelpWindow = "army_screen";

constexpr ButtonSpec own(std::string_view guide, std::string_view group, std::string_view base = {}, std::string_view help = {}) {
    return {.guide = guide, .help_item = help, .art = ButtonArt::faces, .faces = group, .face_base = base};
}

constexpr ButtonSpec kOk          = own("ok_button", "button.ok");
constexpr ButtonSpec kDismiss     = own("dismiss", "button.dismiss");
constexpr ButtonSpec kHidePotions = own("hide_potions_button", "button.hide_Potion", "Show_Potions");
constexpr ButtonSpec kMoveDown    = own("move_army_down", "button.move_down");
constexpr ButtonSpec kMoveUp      = own("move_army_up", "button.move_tombstone_up");
constexpr ButtonSpec kSplit       = own("split_button", "button.split");
constexpr ButtonSpec kSplitSingle = own("single_split_button", "button.split_Single", {}, "split");
constexpr ButtonSpec kSpellbook{.guide = "SpellBook", .art = ButtonArt::authored};
constexpr std::array<ButtonSpec, 2> kArmyArrows{{
    {.guide = "Army_Up", .help_item = "click_army_up", .art = ButtonArt::authored},
    {.guide = "Army_Down", .help_item = "click_army_down", .art = ButtonArt::authored},
}};

struct Formation {
    std::string_view art, help;
    ArmyWindowView::Control control;
};
constexpr std::array<Formation, 3> kFormations{{
    {"loose", "spread", ArmyWindowView::Control::loose},
    {"tight", "tight", ArmyWindowView::Control::tight},
    {"square", "square", ArmyWindowView::Control::square},
}};

}

core::Result<std::unique_ptr<ArmyWindowView>>
ArmyWindowView::build(const assets::Assets& assets, core::Size frame, ArmyWindowModel model) {
    return compose_view<ArmyWindowView>(assets, frame, std::move(model));
}

core::Result<void> ArmyWindowView::compose(const assets::Assets& assets) {
    auto opened = Builder::open(tree().root(), assets, {"dialog.army.layout", kHelpWindow, "army window"});
    if (!opened)
        return std::unexpected{opened.error()};
    const formats::Layer& background = opened->layer("background");
    Builder window = opened->centred("army_window",
                                     HelpDecision::unread("the army window's art (0x527E70)"),
                                     background.bounds().size());
    hold_dialog(window.parent());
    window.sprite_at(core::Point{}, background, HelpDecision::from_parent());
    if (model_.member.has_value())
        page_ = &window.add<MemberPage>(HelpDecision::from_parent(), window, *model_.member);
    add_buttons(window);
    add_rings(window);
    add_army_list(window);
    if (model_.member.has_value())
        numbers_ = &window.add<MemberNumbers>(HelpDecision::from_parent(), window, *model_.member);
    return window.finish();
}

void ArmyWindowView::announce(ui::Button& button, Control control) {
    controls_[std::to_underlying(control)] = &button;
    connections_.relay(button.clicked, button_clicked, control);
}

void ArmyWindowView::add_formations(Builder& window) {
    std::vector<ui::ToggleButton*> toggles;
    std::vector<Control> controls;
    for (const Formation& formation : kFormations) {
        const auto faces = ui::ButtonFaces::from_group(window.group(), formation.art);
        if (!faces)
            continue;
        toggles.push_back(&window.add<ui::ToggleButton>(
            window.help(formation.help), core::Point{}, *faces, formation.control == Control::loose
        ));
        controls.push_back(formation.control);
    }
    formations_ = std::make_unique<ui::RadioGroup>(std::move(toggles), true);
    connections_.on(formations_->selection_changed,
                    [this, controls = std::move(controls)](std::size_t index) { button_clicked.emit(controls[index]); });
}

void ArmyWindowView::add_buttons(Builder& window) {
    ok_ = &window.button(kOk);
    connections_.relay(ok_->clicked, closed);
    add_formations(window);
    if (model_.member.has_value() && !model_.member->hero)
        announce(window.button(kDismiss), Control::dismiss);
    announce(window.button(kHidePotions), Control::hide_potions);
    if (model_.member.has_value() && model_.member->hero)
        announce(window.button(kSpellbook), Control::spellbook);

    if (model_.below.has_value()) {
        announce(window.button(kMoveDown), Control::move_army_down);
        announce(window.button(kMoveUp), Control::move_army_up);
    }
}

void ArmyWindowView::add_rings(Builder& window) {
    const bool two                = model_.below.has_value();
    const formats::Layer& art     = window.layer(two ? "double_ring_background" : "single_ring_background");
    const formats::Layer& circles = window.layer("creature_circles");
    window.sprite(art, HelpDecision::from_parent());
    auto& split                                   = window.button(two ? kSplit : kSplitSingle);
    controls_[std::to_underlying(Control::split)] = &split;
    connections_.relay(split.clicked, button_clicked, Control::split);
    const auto unread = HelpDecision::unread("t_army_display's own words (0x644CD0)");
    rings_ = &window.add<ArmyRingsView>(unread, window, circles.bounds().origin(), two ? RingLayout::top_row : RingLayout::middle_row);
    connections_.relay(rings_->ring_pressed, ring_pressed);
    rings_->set_entries(model_.army);
    rings_->select(model_.selected);
    if (!two)
        return;
    const core::Point below{circles.bounds().left(), circles.bounds().top() + rings_->rings_rect().height()};
    below_ = &window.add<ArmyRingsView>(unread, window, below, RingLayout::bottom_row);
    below_->set_entries(*model_.below);
    window.parent().place_before(*below_, *rings_);
}

void ArmyWindowView::add_army_list(Builder& window) {
    army_list_ = &window.add<HeroListView>(window.help("adventure_map", "army_list"), window, window.guide("army_list"));
    connections_.relay(army_list_->row_pressed, army_pressed);
    army_list_->set_entries(model_.armies);
    army_list_->select(model_.army_row);
    army_list_->hide_rows_from(model_.armies.size());
    if (model_.armies.size() <= army_list_->rows())
        return;
    for (const ButtonSpec& arrow : kArmyArrows)
        if (ui::ButtonFaces::from_group(window.group(), arrow.guide))
            window.button(arrow);
}

void ArmyWindowView::advance(core::Instant now) {
    if (page_ != nullptr)
        if (CreaturePortrait* box = page_->creature_portrait())
            box->advance(now);
}

core::Rect ArmyWindowView::ok_rect() const noexcept {
    return ok_->rect_in_root();
}

core::Rect ArmyWindowView::control_rect(Control control) const noexcept {
    return ui::shown_rect(controls_[std::to_underlying(control)]);
}

}
