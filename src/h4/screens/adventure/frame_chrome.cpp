#include "h4/screens/adventure/frame_chrome.hpp"

#include <algorithm>
#include <array>
#include <string>

#include "h4/core/ascii.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/placement_guide.hpp"
#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::array<std::string_view, 2> kLeftovers{"IGNORE", "DONTUSE"};

constexpr std::string_view kChrome = "the adventure frame's art (0x4A9F10's builder)";

struct IconHelp {
    std::string_view layer;
    std::string_view item;
};
constexpr std::array<IconHelp, 11> kIconHelp{{
    {"Health_Icon", "hit_points"},
    {"Damage_ICon", "damage"},
    {"Move_Icon", "movement"},
    {"Mana_Icon", "spell_points"},
    {"Gold", "gold"},
    {"Wood", "wood"},
    {"Ore", "ore"},
    {"Mercury", "mercury"},
    {"Sulfur", "sulfur"},
    {"Crystal", "crystal"},
    {"Gems", "gems"},
}};

[[nodiscard]] int paint_rank(std::string_view name) noexcept {
    for (const std::string_view strip : {"left", "top", "right", "bottom"})
        if (core::iequals(name, strip))
            return 0;
    if (core::icontains(name, "border"))
        return 1;
    if (core::icontains(name, "background"))
        return 2;
    return 3;
}

[[nodiscard]] bool is_chrome(const formats::Layer& layer) noexcept {
    const std::string_view name = layer.name();
    const bool leftover = std::ranges::any_of(kLeftovers, [name](std::string_view l) { return core::iequals(name, l); });
    return !layer.is_placeholder() && !leftover && !core::iequals(name, kStripPlate) && !formats::is_placement_guide(layer);
}

void stretch_rail(ui::Widget& parent, const formats::Layer& rail, std::int32_t edge, std::int32_t shift) {
    const core::Rect art = rail.bounds();
    const auto chrome    = HelpDecision::unread(kChrome);
    if (shift <= 0) {
        add_window<ui::Sprite>(parent, chrome, rail);
        return;
    }
    const std::int32_t cut   = std::clamp(edge, art.left() + 1, art.right());
    const std::int32_t slice = std::max(1, (cut - art.left()) / 2);
    const auto piece = [&](std::int32_t left, std::int32_t right, std::int32_t source_left) {
        if (right <= left)
            return;
        auto& clip = add_window<ui::Panel>(parent, chrome, core::Rect{{left, art.top()}, {right - left, art.height()}});
        add_window<ui::Sprite>(clip, HelpDecision::from_parent(), rail, core::Point{-source_left, -art.top()});
    };
    piece(art.left(), cut, art.left());
    for (std::int32_t x = cut; x < cut + shift; x += slice)
        piece(x, std::min(x + slice, cut + shift), cut - slice);
    piece(cut + shift, art.right() + shift, cut);
}

}

FrameChrome::FrameChrome(Builder& root, const AdventureLayout& layout)
: Grouping{root, "the frame's art speaks piece by piece"} {
    Builder in = root.inside(*this);
    for (int rank = 0; rank <= 3; ++rank)
        for (const formats::Layer& layer : in.group().layers()) {
            if (!is_chrome(layer) || paint_rank(layer.name()) != rank)
                continue;
            const std::string_view name = layer.name();
            if (spans_map_window(name)) {
                stretch_rail(*this, layer, layout.rail_cut, layout.panel_shift);
                continue;
            }
            const auto icon = std::ranges::find_if(kIconHelp,
                                                   [name](const IconHelp& h) { return core::iequals(name, h.layer); });
            auto& piece = in.add<ui::Sprite>(icon != kIconHelp.end() ? in.help("shared", icon->item) : HelpDecision::unread(kChrome),
                                             layer,
                                             stands_left_of_map(name) ? core::Point{} : core::Point{layout.panel_shift, 0});
            piece.set_name(std::string{name});
        }
}

}
