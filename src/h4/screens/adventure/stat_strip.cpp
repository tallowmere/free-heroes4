#include "h4/screens/adventure/stat_strip.hpp"

#include <format>
#include <utility>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/adventure/meters.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::int32_t kPairSize = 12;

[[nodiscard]] core::Rect meter_window(const formats::BitmapGroup& art, const core::Rect& guide) {
    return art.bounds().translated(guide.origin());
}

}

StatStrip::StatStrip(Builder& root, const AdventureLayout& layout, const formats::Layer* plate)
: Grouping{root, "the strip's pieces speak for themselves"}, assets_{&root.assets()} {
    if (layout.creature_name.empty())
        return;
    Builder b        = root.inside(*this);
    const auto words = HelpDecision::unread("the strip's text windows (0x4ACBB0)");
    name_       = &b.label(layout.creature_name, {}, ui::kEngravedLeft, words);
    class_line_ = &b.label(layout.hero_class, {}, ui::kEngravedLeft, words);
    const std::array<std::pair<core::Rect, std::int32_t>, 4> numbers{{
        {layout.creature_health, 0},
        {layout.creature_damage, 0},
        {layout.creature_move, kPairSize},
        {layout.creature_mana, kPairSize},
    }};
    for (std::size_t i = 0; i < numbers.size(); ++i)
        numbers_[i] = &b.label(numbers[i].first, {}, ui::kEngravedCentred, words, numbers[i].second);
    const auto empty_cell = HelpDecision::unread("an empty strip cell's parent (0x851F20)");
    const auto cell       = [&](core::Rect rect) -> ui::IconCell& {
        return b.add<ui::IconCell>(empty_cell, rect, nullptr, nullptr, ui::IconCell::Placement::authored);
    };
    for (std::size_t i = 0; i < icons_.size(); ++i)
        icons_[i] = &cell(layout.creature_skills[i]);
    luck_art_   = &b.group(meter_group(layout.luck.height()));
    morale_art_ = &b.group(meter_group(layout.morale.height()));
    luck_       = &cell(meter_window(*luck_art_, layout.luck));
    morale_     = &cell(meter_window(*morale_art_, layout.morale));
    if (plate != nullptr)
        b.add<ui::Sprite>(HelpDecision::unread("the strip's plate (0x4ACBB0)"), *plate, core::Point{layout.panel_shift, 0})
            .set_name(std::string{plate->name()});
    set_entry({});
}

void StatStrip::set_entry(const StatEntry& entry) {
    if (!built())
        return;
    name_->set_text(entry.shown ? entry.name : std::string{});
    class_line_->set_text(entry.shown ? entry.class_line : std::string{});
    const std::array<const std::string*, 4> values{&entry.health, &entry.damage, &entry.move, &entry.mana};
    for (std::size_t i = 0; i < numbers_.size(); ++i)
        numbers_[i]->set_text(entry.shown ? *values[i] : std::string{});
    for (std::size_t i = 0; i < icons_.size(); ++i) {
        const NamedIcon& icon = entry.shown ? entry.icons[i] : NamedIcon{};
        icons_[i]->set_layer(icon.base, ui::IconCell::Placement::authored);
        icons_[i]->set_overlay(icon.overlay);
        icons_[i]->set_help(icon.help);
    }
    const auto meter = [&](ui::IconCell& cell, const formats::BitmapGroup* art, Meter which, const std::optional<int>& value) {
        if (entry.shown && value.has_value()) {
            cell.set_layer(meter_layer(art, which, *value), ui::IconCell::Placement::authored);
            cell.set_help(meter_help(*assets_, which, *value));
        } else {
            cell.set_layer(nullptr, ui::IconCell::Placement::authored);
        }
    };
    meter(*luck_, luck_art_, Meter::luck, entry.luck);
    meter(*morale_, morale_art_, Meter::morale, entry.morale);
}

std::string_view StatStrip::text(std::size_t which) const noexcept {
    if (!built())
        return {};
    if (which == 0)
        return name_->text();
    if (which == 1)
        return class_line_->text();
    return which - 2 < numbers_.size() ? numbers_[which - 2]->text() : std::string_view{};
}

}
