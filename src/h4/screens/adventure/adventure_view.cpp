#include "h4/screens/adventure/adventure_view.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <string>

#include "h4/assets/assets.hpp"
#include "h4/model/map.hpp"
#include "h4/screens/adventure/day_scroll.hpp"
#include "h4/screens/adventure/frame_buttons.hpp"
#include "h4/screens/adventure/frame_chrome.hpp"
#include "h4/screens/adventure/resource_strip.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kHelpWindow = "adventure_map";

}

core::Result<std::unique_ptr<AdventureView>> AdventureView::build(const assets::Assets& assets, core::Size frame) {
    return compose_view<AdventureView>(assets, frame);
}

core::Result<void> AdventureView::compose(const assets::Assets& assets) {
    const core::Size frame = frame_size();
    const std::string name = std::format("adventure.{}", variant_suffix(variant_for_height(frame.height)));
    auto opened            = Builder::open(tree().root(), assets, {name, kHelpWindow, "adventure"});
    if (!opened)
        return std::unexpected{opened.error()};
    Builder& root = *opened;
    auto layout   = AdventureLayout::from_group(root.group(), frame.width);
    if (!layout)
        return std::unexpected{layout.error()};
    layout_ = *layout;

    map_window_ = &root.add<MapWindow>(HelpDecision::silent("no map is shown yet: its words are its map's"),
                                       layout_.viewport,
                                       assets.font_for_size(25));
    map_window_->set_name("map");
    root.add<FrameChrome>(HelpDecision::from_parent(), root, layout_);
    mini_map_ = &root.add<MiniMapWindow>(root.help("mini_map"), layout_.mini_map, *map_window_);
    mini_map_->set_name("mini_map");

    hero_list_  = &root.add<HeroListView>(root.help("army_list"), root, layout_.hero_list);
    town_list_  = &root.add<TownListView>(root.help("town_list"), root, layout_.town_list);
    strip_      = &root.add<StatStrip>(HelpDecision::from_parent(), root, layout_, root.find(kStripPlate));
    army_rings_ = &root.add<ArmyRingsView>(HelpDecision::unread("t_army_display's own words (0x644CD0)"),
                                           root,
                                           layout_.army.origin());
    buttons_    = &root.add<FrameButtons>(HelpDecision::from_parent(), root, layout_);
    resources_  = &root.add<ResourceStrip>(HelpDecision::from_parent(), root, layout_);
    day_ = &root.add<DayScroll>(root.help("date"), root, layout_.day_scroll.origin());
    connect_parts();
    return root.finish();
}

void AdventureView::connect_parts() {
    connections_.relay(map_window_->pointed, map_pointed);
    connections_.relay(map_window_->clicked, map_clicked);
    connections_.relay(mini_map_->pointed, mini_map_pointed);
    connections_.relay(hero_list_->row_pressed, hero_row_pressed);
    connections_.relay(hero_list_->row_double_clicked, hero_row_double_clicked);
    connections_.relay(town_list_->row_pressed, town_row_pressed);
    connections_.relay(army_rings_->ring_pressed, ring_pressed);
    connections_.relay(army_rings_->ring_double_clicked, ring_double_clicked);
    connections_.relay(buttons_->clicked, button_clicked);
}

void AdventureView::bind(const AdventureFrameModel& model) {
    hero_list_->set_entries(model.heroes);
    hero_list_->select(model.hero_selected);
    town_list_->set_entries(model.towns);
    town_list_->select(model.town_selected);
    army_rings_->set_entries(model.rings);
    army_rings_->select(model.ring_selected);
    strip_->set_entry(model.strip);
    for (std::size_t i = 0; i < kAdventureResources; ++i)
        resources_->set_amount(static_cast<AdventureResource>(i), model.resources[i]);
    day_->set_day(model.day);
}

void AdventureView::show_map(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen) {
    map_window_->show_map(assets, map, seen);
    mini_map_->show_map(assets, map);
    town_thumbnail_.emplace(assets, map, *map_window_->renderer(), town_list_->thumbnail_size());
    town_list_->set_map(&*town_thumbnail_, map.dimension());
}

core::Rect AdventureView::button_rect(AdventureButton button) const noexcept {
    return buttons_->button_rect(button);
}

std::string_view AdventureView::day_text() const noexcept {
    return day_->day_text();
}

std::string_view AdventureView::resource_text(AdventureResource resource) const noexcept {
    return resources_->amount(resource);
}

void AdventureView::advance(core::Instant now) {
    set_clock(static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()));
    ticked.emit(clock_ms_);
}

void AdventureView::set_clock(std::uint32_t clock_ms) {
    clock_ms_ = clock_ms;
    map_window_->set_clock(clock_ms_);
}

}
