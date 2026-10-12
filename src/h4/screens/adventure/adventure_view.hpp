#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/signal.hpp"
#include "h4/scene/mini_map.hpp"
#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/adventure/army_rings_view.hpp"
#include "h4/screens/adventure/hero_list_view.hpp"
#include "h4/screens/adventure/map_window.hpp"
#include "h4/screens/adventure/mini_map_window.hpp"
#include "h4/screens/adventure/stat_strip.hpp"
#include "h4/screens/adventure/town_list_view.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"

namespace h4::assets {
class Assets;
struct AssetRequest;
}
namespace h4::model {
class Map;
class Shroud;
}

namespace h4::screens {

class DayScroll;
class FrameButtons;
class ResourceStrip;

struct AdventureFrameModel {
    std::span<const HeroListEntry> heroes;
    std::optional<std::size_t> hero_selected;
    std::span<const TownListEntry> towns;
    std::optional<std::size_t> town_selected;
    std::span<const ArmyRingEntry> rings;
    std::optional<std::size_t> ring_selected;
    StatEntry strip;
    std::array<std::string, kAdventureResources> resources;
    std::string day;
};

class AdventureView final : public WidgetView<View> {
    friend ViewFactory;
    explicit AdventureView(core::Size frame) : WidgetView{frame} {
    }

public:
    [[nodiscard]] static core::Result<std::unique_ptr<AdventureView>> build(const assets::Assets& assets, core::Size frame);
    [[nodiscard]] static std::vector<assets::AssetRequest> assets_needed(core::Size frame);

    void bind(const AdventureFrameModel& model);

    void show_map(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen = nullptr);
    [[nodiscard]] bool has_map() const noexcept {
        return map_window_->renderer() != nullptr;
    }

    core::Signal<AdventureButton> button_clicked;
    core::Signal<core::Point> mini_map_pointed;
    core::Signal<std::size_t> ring_pressed;
    core::Signal<std::size_t> ring_double_clicked;
    core::Signal<std::size_t> hero_row_pressed;
    core::Signal<std::size_t> town_row_pressed;
    core::Signal<std::size_t> hero_row_double_clicked;
    core::Signal<scene::MapHover> map_pointed;
    core::Signal<scene::MapHover> map_clicked;
    core::Signal<std::uint32_t> ticked;

    [[nodiscard]] MapWindow& map_window() noexcept {
        return *map_window_;
    }
    [[nodiscard]] const MapWindow& map_window() const noexcept {
        return *map_window_;
    }
    [[nodiscard]] const HeroListView& hero_list() const noexcept {
        return *hero_list_;
    }
    [[nodiscard]] const TownListView& town_list() const noexcept {
        return *town_list_;
    }
    [[nodiscard]] const ArmyRingsView& army_rings() const noexcept {
        return *army_rings_;
    }
    [[nodiscard]] const StatStrip& strip() const noexcept {
        return *strip_;
    }

    [[nodiscard]] const AdventureLayout& layout() const noexcept {
        return layout_;
    }
    [[nodiscard]] core::Rect viewport_rect() const noexcept {
        return layout_.viewport;
    }
    [[nodiscard]] core::Rect button_rect(AdventureButton button) const noexcept;
    [[nodiscard]] std::string_view day_text() const noexcept;
    [[nodiscard]] std::string_view resource_text(AdventureResource resource) const noexcept;

    void advance(core::Instant now) override;
    [[nodiscard]] std::uint32_t clock_ms() const noexcept {
        return clock_ms_;
    }
    void set_clock(std::uint32_t clock_ms);

private:
    [[nodiscard]] core::Result<void> compose(const assets::Assets& assets);
    void connect_parts();

    AdventureLayout layout_;
    MapWindow* map_window_     = nullptr;
    MiniMapWindow* mini_map_   = nullptr;
    HeroListView* hero_list_   = nullptr;
    TownListView* town_list_   = nullptr;
    StatStrip* strip_          = nullptr;
    ArmyRingsView* army_rings_ = nullptr;
    FrameButtons* buttons_     = nullptr;
    ResourceStrip* resources_  = nullptr;
    DayScroll* day_            = nullptr;
    std::optional<scene::MiniMap> town_thumbnail_;
    std::uint32_t clock_ms_ = 0;
    core::Connections connections_;
};

}
