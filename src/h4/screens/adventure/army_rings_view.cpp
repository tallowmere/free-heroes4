#include "h4/screens/adventure/army_rings_view.hpp"

#include <format>
#include <string>
#include <utility>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/adventure/row_strip.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/click_area.hpp"
#include "h4/ui/widgets/icon_cell.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/meter.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

struct Cell {
    int a, b;
};
using Layout = std::array<Cell, kArmyRings>;
constexpr std::array<Layout, 4> kLayouts{{
    {{{1, 0}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}}},
    {{{0, 0}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 2}}},
    {{{2, 0}, {2, 1}, {2, 1}, {2, 1}, {2, 1}, {2, 1}, {2, 2}}},
    {{{0, 0}, {2, 0}, {0, 1}, {2, 1}, {0, 1}, {2, 2}, {1, 2}}},
}};
constexpr std::array<std::string_view, 9> kShapes{"top_left", "top", "top_right", "left", "middle", "right", "bottom_left", "bottom", "bottom_right"};

[[nodiscard]] std::string_view shape_of(const Layout& layout, std::size_t slot) {
    return kShapes[static_cast<std::size_t>(layout[slot].a * 3 + layout[slot].b)];
}

[[nodiscard]] std::array<core::Point, kArmyRings> walk(Builder& art, RingLayout ring_layout) {
    const Layout& layout           = kLayouts[std::to_underlying(ring_layout)];
    const bool pairs               = ring_layout == RingLayout::panel;
    const formats::Layer& top_left = art.layer(kShapes[0]);
    std::array<core::Point, kArmyRings> offsets{};
    core::Point cursor{};
    for (std::size_t i = 0; i < kArmyRings; ++i) {
        if (i > 0 && !pairs)
            cursor.x += art.layer(shape_of(layout, i - 1)).bounds().width();
        else if (i > 0 && layout[i - 1].a == 0)
            cursor.y += art.layer(shape_of(layout, i - 1)).bounds().height();
        else if (i > 0) {
            cursor.x += art.layer(shape_of(layout, i - 2)).bounds().width();
            cursor.y -= top_left.bounds().height();
        }
        offsets[i] = cursor - art.layer(shape_of(layout, i)).bounds().origin();
    }
    return offsets;
}

}

ArmyRingsView::BannerArt ArmyRingsView::BannerArt::load(Builder& art) {
    const formats::Layer& text = art.layer("inset_text");
    return BannerArt{&art.layer("inset"),
                     &text,
                     &art.layer("health_bar"),
                     &art.layer("health_bar_border"),
                     &art.layer("mana_bar"),
                     &art.layer("mana_bar_border"),
                     &art.font(text.bounds().height())};
}

ArmyRingsView::ArmyRingsView(Builder& builder, core::Point anchor, RingLayout layout) : Widget{core::Rect{anchor, {}}} {
    set_name("army");
    Builder art        = builder.over("creature_rings", *this);
    const auto offsets = walk(art, layout);
    for (std::size_t i = 0; i < kArmyRings; ++i)
        add_ring(art, i, shape_of(kLayouts[std::to_underlying(layout)], i), offsets[i]);
    const BannerArt banner = BannerArt::load(art);
    for (std::size_t i = 0; i < kArmyRings; ++i)
        add_banner(banner, i, offsets[i]);
    set_entries({});
    fit(anchor);
}

void ArmyRingsView::add_ring(Builder& art, std::size_t index, std::string_view shape, core::Point offset) {
    const formats::Layer& frame = art.layer(shape);
    Slot& slot                  = slots_[index];
    slot.button = &add_window<ui::ClickArea>(*this,
                                             HelpDecision::unread("an empty ring (0x644CD0's empty arm)"),
                                             frame.bounds().translated(offset));
    const core::Point inside_ring = core::Point{} - frame.bounds().origin();
    const auto piece              = HelpDecision::from_parent();
    slot.portrait                 = &add_window<ui::IconCell>(
        *slot.button, piece, art.layer("portrait").bounds().translated(inside_ring), nullptr, nullptr, ui::IconCell::Placement::at_origin
    );
    slot.plain = &add_window<ui::Sprite>(*slot.button, piece, frame, inside_ring);
    slot.highlight = &add_window<ui::Sprite>(*slot.button, piece, art.layer(std::format("{}_highlight", shape)), inside_ring);
    slot.highlight->set_visible(false);
    add_window<ui::Sprite>(*slot.button, piece, art.layer(std::format("{}_disabled", shape)), inside_ring).set_visible(false);
    connections_.relay(slot.button->pressed, ring_pressed, index);
    connections_.relay(slot.button->double_clicked, ring_double_clicked, index);
}

void ArmyRingsView::add_banner(const BannerArt& art, std::size_t index, core::Point offset) {
    Slot& slot               = slots_[index];
    const core::Point inside = core::Point{} - art.inset->bounds().origin();
    slot.inset               = &add_window<ui::Panel>(*this,
                                                      HelpDecision::unread("an empty ring (0x644CD0's empty arm)"),
                                                      art.inset->bounds().translated(offset));
    const auto piece         = HelpDecision::from_parent();
    add_window<ui::Sprite>(*slot.inset, piece, *art.inset, inside);
    slot.count = &add_window<ui::Label>(
        *slot.inset, piece, art.text->bounds().translated(inside), std::string{}, *art.font, ui::kEngravedCentred
    );
    const auto bar = [&](const formats::Layer& layer, const formats::Layer& edge, ui::Widget*& border) {
        auto& meter = add_window<ui::Meter>(
            *slot.inset, piece, layer.bounds().translated(inside), ui::Meter::Direction::rightwards, ui::Meter::Art{{&layer}}
        );
        border = &add_window<ui::Sprite>(*slot.inset, piece, edge, inside);
        return &meter;
    };
    slot.mana   = bar(*art.mana, *art.mana_edge, slot.mana_edge);
    slot.health = bar(*art.health, *art.health_edge, slot.health_edge);
}

void ArmyRingsView::fit(core::Point anchor) {
    core::Rect extent;
    for (const auto& child : children())
        if (child->visible())
            extent = extent.empty() ? child->rect() : extent.united(child->rect());
    rings_size_        = extent.size();
    core::Rect painted = extent;
    for (const Slot& slot : slots_)
        painted = painted.united(slot.inset->rect());
    set_rect(core::Rect{anchor, {painted.right(), painted.bottom()}});
}

void ArmyRingsView::set_entries(std::span<const ArmyRingEntry> entries) {
    for (std::size_t i = 0; i < kArmyRings; ++i) {
        Slot& slot                = slots_[i];
        const ArmyRingEntry entry = i < entries.size() ? entries[i] : ArmyRingEntry{};
        slot.portrait->set_layer(entry.face, ui::IconCell::Placement::at_origin);
        slot.button->set_help(entry.help);
        slot.inset->set_help(entry.help);
        slot.inset->set_visible(entry.kind != ArmyRingEntry::Kind::empty);
        const bool hero = entry.kind == ArmyRingEntry::Kind::hero;
        slot.count->set_text(entry.count);
        for (ui::Widget* bar :
             {static_cast<ui::Widget*>(slot.health), static_cast<ui::Widget*>(slot.mana), slot.health_edge, slot.mana_edge})
            bar->set_visible(hero);
        slot.health->set_fraction(entry.health, entry.health_max);
        slot.mana->set_fraction(entry.mana, entry.mana_max);
    }
}

void ArmyRingsView::select(std::optional<std::size_t> slot) {
    select_row(slots_, slot, &Slot::plain, &Slot::highlight);
}

core::Rect ArmyRingsView::rings_rect() const noexcept {
    return core::Rect{rect().origin(), rings_size_};
}

core::Rect ArmyRingsView::ring_rect(std::size_t slot) const noexcept {
    return slot < kArmyRings ? slots_[slot].button->rect().translated(rect().origin()) : core::Rect{};
}

std::string_view ArmyRingsView::count_text(std::size_t slot) const noexcept {
    return slots_[slot].count->text();
}

}
