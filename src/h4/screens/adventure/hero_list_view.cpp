#include "h4/screens/adventure/hero_list_view.hpp"

#include <string>
#include <string_view>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/adventure/row_strip.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/click_area.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/meter.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kArt = "control.army_rings";

constexpr int kMoveScale     = 1500;
constexpr int kStrengthScale = 14;
constexpr std::string_view kBarHelp      = "shared";
constexpr std::string_view kStrengthHelp = "army_ring_strength";
constexpr std::int32_t kInterlock = 4;

[[nodiscard]] core::Rect list_rect(Builder& builder, core::Rect guide) {
    return core::Rect{guide.origin(), {builder.layer(kArt, "Army_Frame").bounds().width(), guide.height()}};
}

}

HeroListView::HeroListView(Builder& builder, core::Rect guide)
: Widget{list_rect(builder, guide)}, assets_{&builder.assets()} {
    set_name("hero_list");
    Builder art                     = builder.over(kArt, *this);
    const formats::Layer& frame     = art.layer("Army_Frame");
    const formats::Layer& highlight = art.layer("Army_Frame_Highlight");
    const formats::Layer& empty     = art.layer("Empty_Background");
    const formats::Layer& move      = art.layer("Move_Bar");
    const formats::Layer& strength  = art.layer("Mana_Bar");
    const formats::Layer& portrait  = art.layer("portrait");

    const auto piece         = HelpDecision::from_parent();
    const std::int32_t pitch = frame.bounds().height() - kInterlock;
    for (std::int32_t k = 0; (k + 1) * pitch <= guide.height(); ++k) {
        const core::Point offset{0, k * pitch};
        Row row;
        row.empty = &add_window<ui::Sprite>(*this, piece, empty, offset);
        const auto bar = [&](const formats::Layer& layer) {
            return &add_window<ui::Meter>(
                *this, piece, layer.bounds().translated(offset), ui::Meter::Direction::upwards, ui::Meter::Art{{&layer}}
            );
        };
        row.move     = bar(move);
        row.strength = bar(strength);
        row.strength->set_help(interface_help(*assets_, kBarHelp, kStrengthHelp));
        row.portrait = &add_window<ui::ClickArea>(*this, piece, portrait.bounds().translated(offset));
        row.face     = &add_window<ui::IconCell>(
            *row.portrait, piece, core::Rect{{}, portrait.bounds().size()}, nullptr, nullptr, ui::IconCell::Placement::at_origin
        );
        row.frame               = &add_window<ui::Sprite>(*this, piece, frame, offset);
        row.highlight           = &add_window<ui::Sprite>(*this, piece, highlight, offset);
        const std::size_t index = rows_.size();
        connections_.relay(row.portrait->pressed, row_pressed, index);
        connections_.relay(row.portrait->double_clicked, row_double_clicked, index);
        rows_.push_back(row);
    }
    set_entries({});
    select(std::nullopt);
}

void HeroListView::hide_rows_from(std::size_t first) {
    for (std::size_t i = first; i < rows_.size(); ++i) {
        const Row& row = rows_[i];
        for (ui::Widget* part : {row.empty, row.frame, row.highlight})
            part->set_visible(false);
        row.portrait->set_visible(false);
        row.move->set_visible(false);
        row.strength->set_visible(false);
    }
}

void HeroListView::set_entries(std::span<const HeroListEntry> entries) {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        Row& row                   = rows_[i];
        const HeroListEntry* entry = i < entries.size() ? &entries[i] : nullptr;
        row.face->set_layer(entry != nullptr ? entry->face : nullptr, ui::IconCell::Placement::at_origin);
        row.empty->set_visible(entry == nullptr || entry->face == nullptr);
        row.portrait->set_visible(entry != nullptr);
        row.move->set_visible(entry != nullptr);
        row.strength->set_visible(entry != nullptr);
        row.portrait->set_help(entry != nullptr ? entry->help : ui::Help{});
        if (entry == nullptr)
            continue;
        row.move->set_fraction(entry->move, kMoveScale);
        row.strength->set_fraction(entry->strength, kStrengthScale);
        row.move->set_help(entry->move_help);
    }
}

void HeroListView::select(std::optional<std::size_t> row) {
    select_row(rows_, row, &Row::frame, &Row::highlight);
}

core::Rect HeroListView::row_rect(std::size_t row) const noexcept {
    return row < rows_.size() ? rows_[row].portrait->rect().translated(rect().origin()) : core::Rect{};
}

core::Rect HeroListView::move_rect(std::size_t row) const noexcept {
    return row < rows_.size() ? rows_[row].move->rect().translated(rect().origin()) : core::Rect{};
}

core::Rect HeroListView::strength_rect(std::size_t row) const noexcept {
    return row < rows_.size() ? rows_[row].strength->rect().translated(rect().origin()) : core::Rect{};
}

}
