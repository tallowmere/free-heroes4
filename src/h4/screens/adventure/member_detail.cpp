#include "h4/screens/adventure/member_detail.hpp"

#include <format>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/adventure/creature_portrait.hpp"
#include "h4/screens/adventure/meters.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/control.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

ui::LabelStyle style(graphics::HorizontalAlign across, graphics::VerticalAlign down) {
    return ui::LabelStyle{graphics::TextStyle::engraved(), {across, down}};
}

ui::IconCell& icon_cell(ui::Widget& parent, core::Rect rect, const NamedIcon& icon) {
    auto& cell = add_window<ui::IconCell>(
        parent, HelpDecision::from_parent(), rect, icon.base, icon.overlay, ui::IconCell::Placement::authored
    );
    cell.set_help(icon.help);
    return cell;
}

constexpr std::array<std::string_view, 14>
    kWornNames{"bow", "feet", "head", "left ring", "misc_1", "misc_2", "misc_3", "misc_4", "neck", "right ring", "left hand", "shoulders", "torso", "right hand"};

constexpr std::string_view kShared = "shared";
constexpr std::string_view kArmy   = "army_screen";

}

const std::array<MemberNumbers::Number, 13> MemberNumbers::kNumbers{{
    {"damage", kShared, "damage", "damage_text", &ArmyWindowMember::damage, false, 14},
    {"hit points", kShared, "hit_points", "hit_points_text", &ArmyWindowMember::hit_points, false, 14},
    {"move", kShared, "adventure_movement", "move_text", &ArmyWindowMember::move, false, 14},
    {"speed", kShared, "speed", "speed_text", &ArmyWindowMember::speed, false, 14},
    {"experience", kShared, "experience", "experience_text", &ArmyWindowMember::experience, true, 0},
    {"shots", kShared, "shots", "shots_text", &ArmyWindowMember::shots, false, 14},
    {"melee_attack", kArmy, "melee_attack", "melee_attack_text", &ArmyWindowMember::melee_attack, false, 14},
    {"melee_defense", kArmy, "melee_defense", "melee_defense_text", &ArmyWindowMember::melee_defense, false, 14},
    {"ranged_attack", kArmy, "ranged_attack", "ranged_attack_text", &ArmyWindowMember::ranged_attack, false, 14},
    {"ranged_defense", kArmy, "ranged_defense", "ranged_defense_text", &ArmyWindowMember::ranged_defense, false, 14},
    {"spell_points", kShared, "spell_points", "spell_points_text", &ArmyWindowMember::spell_points, false, 14},
    {{}, {}, {}, "morale_text", &ArmyWindowMember::morale, false, 0},
    {{}, {}, {}, "luck_text", &ArmyWindowMember::luck, false, 0},
}};

MemberPage::MemberPage(Builder& window, const ArmyWindowMember& member)
: Grouping{window, "the page's pieces speak for themselves"} {
    Builder page = window.inside(*this);
    if (member.hero)
        add_hero(page, member);
    else
        add_creature(page, member);
}

void MemberPage::add_hero(Builder& page, const ArmyWindowMember& hero) {
    if (const formats::Layer* art = page.find("hero_background"))
        page.sprite(*art, HelpDecision::from_parent());
    const core::Point doll_at = page.marker("hero_inventory");
    if (hero.doll != nullptr) {
        if (const formats::Layer* doll = hero.doll->find("background"))
            page.sprite_at(doll_at, *doll, HelpDecision::from_parent());
        for (std::size_t slot = 0; slot < kWornNames.size(); ++slot) {
            const formats::Layer* box = hero.doll->find(kWornNames[slot]);
            if (box == nullptr)
                continue;
            worn_rects_[slot]     = box->bounds().translated(doll_at);
            const NamedIcon& icon = hero.worn[slot];
            if (icon.base != nullptr)
                icon_cell(*this, worn_rects_[slot], icon);
        }
    }
    for (std::size_t i = 0; i < hero.backpack.size(); ++i) {
        const formats::Layer* box = page.find(std::format("backpack {} slot", i + 1));
        const NamedIcon& icon     = hero.backpack[i];
        if (box != nullptr && icon.base != nullptr)
            icon_cell(*this, box->bounds(), icon);
    }
    add_grid(page, hero, "skill_frame");
    add_portrait(page, hero);
}

void MemberPage::add_creature(Builder& page, const ArmyWindowMember& creature) {
    if (const formats::Layer* art = page.find("creature_background"))
        page.sprite(*art, HelpDecision::from_parent());
    add_grid(page, creature, "abilities_frame");
    if (const formats::Layer* box = page.find("creature_box"))
        creature_portrait_ = &page.add<CreaturePortrait>(
            HelpDecision::from_parent(), box->bounds(), page.assets(), CreatureBoxModel{creature.model, creature.alignment}
        );
    if (const formats::Layer* guide = page.find("creature_text"))
        page.label(guide->bounds(), creature.text, ui::kEngravedLeft, HelpDecision::from_parent(), 20);
    add_portrait(page, creature);
}

void MemberPage::add_grid(Builder& page, const ArmyWindowMember& member, std::string_view frame) {
    std::array<const formats::Layer*, 4> columns{};
    for (std::size_t c = 0; c < columns.size(); ++c)
        columns[c] = page.find(std::format("skill_{}", c + 1));
    for (std::size_t row = 0; row < member.skills.size(); ++row) {
        const formats::Layer* line = row == 0 ? columns[0] : page.find(std::format("skill_row_{}", row + 1));
        for (std::size_t c = 0; c < columns.size(); ++c) {
            if (line == nullptr || columns[c] == nullptr)
                continue;
            const core::Rect cell{{columns[c]->bounds().left(), line->bounds().top()}, columns[c]->bounds().size()};
            skill_rects_[row][c]  = cell;
            const NamedIcon& icon = member.skills[row][c];
            if (icon.base != nullptr)
                icon_cell(*this, cell, icon);
        }
    }
    if (const formats::Layer* art = page.find(frame))
        page.sprite(*art, HelpDecision::from_parent());
}

void MemberPage::add_portrait(Builder& page, const ArmyWindowMember& member) {
    if (const formats::Layer* guide = page.find("creature_portrait"); guide != nullptr && member.portrait != nullptr)
        add_control(*this, HelpDecision::from_parent(), *member.portrait, guide->bounds().origin());
    if (const formats::Layer* border = page.find("portrait_border"))
        page.sprite(*border, HelpDecision::from_parent());
}

core::Rect MemberPage::worn_rect(std::size_t slot) const noexcept {
    return slot < worn_rects_.size() && !worn_rects_[slot].empty() ? worn_rects_[slot].translated(origin_in_root()) :
                                                                     core::Rect{};
}

core::Rect MemberPage::skill_rect(std::size_t row, std::size_t column) const noexcept {
    return row < 5 && column < 4 && !skill_rects_[row][column].empty() ?
        skill_rects_[row][column].translated(origin_in_root()) :
        core::Rect{};
}

MemberNumbers::MemberNumbers(Builder& outer, const ArmyWindowMember& member)
: Grouping{outer, "each number speaks by its icon"} {
    Builder window = outer.inside(*this);
    for (const Number& number : kNumbers)
        add_number(window, number, member);
    add_meter(window, Meter::morale, member.morale_value);
    add_meter(window, Meter::luck, member.luck_value);
    add_line(window, "name_text", member.name);
    add_line(window, "class_text", member.class_line);
}

void MemberNumbers::add_number(Builder& window, const Number& number, const ArmyWindowMember& member) {
    const std::string& words = member.*number.words;
    if (number.words == &ArmyWindowMember::spell_points && words.empty())
        return;
    if (!number.icon.empty())
        if (const formats::Layer* icon = window.find(number.icon))
            window.sprite(*icon, window.help(number.window, number.help));
    const formats::Layer* guide = window.find(number.guide);
    if (guide == nullptr || (number.hero_only && !member.hero))
        return;
    window.label(guide->bounds(),
                 words,
                 style(graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre),
                 HelpDecision::from_parent(),
                 number.size);
    texts_.push_back({std::string{number.guide}, words});
}

void MemberNumbers::add_meter(Builder& window, Meter which, std::optional<int> value) {
    const formats::Layer* guide = window.find(which == Meter::luck ? "luck" : "morale");
    if (guide == nullptr || !value.has_value())
        return;
    const formats::BitmapGroup* art = window.assets().group(meter_group(guide->bounds().height()));
    if (const formats::Layer* icon = meter_layer(art, which, *value))
        window.add<ui::Sprite>(HelpDecision::words(meter_help(window.assets(), which, *value)), *icon, guide->bounds().origin());
}

void MemberNumbers::add_line(Builder& window, std::string_view guide, const std::string& words) {
    const formats::Layer* at = window.find(guide);
    if (at == nullptr || words.empty())
        return;
    window.label(at->bounds(), words, style(graphics::HorizontalAlign::left, graphics::VerticalAlign::top), HelpDecision::from_parent(), 16);
    texts_.push_back({std::string{guide}, words});
}

std::string_view MemberNumbers::text_at(std::string_view guide) const noexcept {
    for (const Text& text : texts_)
        if (text.guide == guide)
            return text.words;
    return {};
}

}
