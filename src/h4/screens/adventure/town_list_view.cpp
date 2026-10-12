#include "h4/screens/adventure/town_list_view.hpp"

#include <format>
#include <string>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/scene/mini_map.hpp"
#include "h4/screens/adventure/row_strip.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/box.hpp"
#include "h4/ui/widgets/click_area.hpp"
#include "h4/ui/widgets/painting.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kArt = "town.small_list";

constexpr std::int32_t kRowGap = 2;

constexpr std::array<std::string_view, 3> kFamilies{"misc", "magic", "creatures"};

void compose(const assets::Assets& assets, const TownListEntry& entry, graphics::Surface565& surface) {
    surface.fill({});
    const std::string name = entry.alignment.has_value() ? std::format("town.{}.tiny", model::to_string(*entry.alignment)) :
                                                           std::string{"town.random.89"};
    const formats::BitmapGroup* group = assets.group(name);
    if (group == nullptr)
        return;
    const graphics::Canvas canvas{surface};
    if (const formats::Layer* ground = group->find(entry.ground))
        canvas.draw_layer(*ground);
    if (const formats::Layer* fort = group->find(entry.fortification))
        canvas.draw_layer(*fort);
    if (!entry.alignment.has_value())
        return;

    const formats::BitmapGroup* graph = assets.group("town.graph");
    if (graph == nullptr)
        return;
    for (std::size_t i = 0; i < kFamilies.size(); ++i) {
        const formats::Layer* track  = group->find(kFamilies[i]);
        const formats::Layer* fill   = graph->find(kFamilies[i]);
        const formats::Layer* shadow = graph->find(std::format("{} shadow", kFamilies[i]));
        if (track == nullptr || fill == nullptr || shadow == nullptr)
            continue;
        const TownBar& bar        = entry.bars[i];
        const core::Rect rail     = track->bounds();
        const std::int32_t length = bar.total <= 0 ? 0 : (rail.width() - 1) * bar.built / bar.total;
        const core::Point at = rail.origin();
        canvas.clipped_to(core::Rect::from_edges(1, 1, length + 1, rail.height()).translated(at)).draw_layer(*shadow, at);
        canvas.clipped_to(core::Rect::from_edges(0, 0, length, rail.height() - 1).translated(at)).draw_layer(*fill, at);
    }
}

[[nodiscard]] core::Rect list_rect(Builder& builder, core::Rect guide) {
    return core::Rect{guide.origin(), {builder.layer(kArt, "Frame").bounds().width(), guide.height()}};
}

}

TownListView::TownListView(Builder& builder, core::Rect guide)
: Widget{list_rect(builder, guide)}, assets_{&builder.assets()} {
    set_name("town_list");
    Builder art                        = builder.over(kArt, *this);
    const formats::Layer& frame        = art.layer("Frame");
    const formats::Layer& highlight    = art.layer("Highlighted");
    const formats::Layer& built        = art.layer("Built");
    const formats::Layer& town         = art.layer("town");
    const formats::Layer& map          = art.layer("map");
    marker_                            = &art.layer("marker");
    const formats::BitmapGroup* ground = assets_->group("town.random.89");
    const core::Size picture           = ground != nullptr ? ground->bounds().size() : core::Size{89, 48};
    thumbnail_size_                    = map.bounds().size();

    const auto piece                = HelpDecision::from_parent();
    const std::int32_t frame_height = frame.bounds().height();
    const std::int32_t pitch        = frame_height + kRowGap;
    for (std::int32_t k = 0; k * pitch + frame_height <= guide.height(); ++k) {
        const core::Point offset{0, k * pitch};
        Row row;
        row.picture = std::make_unique<graphics::Surface565>(picture);
        row.picture_window = &add_window<ui::ClickArea>(*this, piece, core::Rect{town.bounds().origin() + offset, picture});
        add_window<ui::Painting>(*row.picture_window, piece, *row.picture, core::Point{});
        const core::Rect thumb = map.bounds().translated(offset);
        row.map                = &add_window<ui::Panel>(*this, piece, thumb);
        const core::Rect local{{}, thumb.size()};
        add_window<ui::Box>(*row.map, piece, local, graphics::Rgba{0, 0, 0, 255}, std::nullopt);
        row.map_image = &add_window<ui::Painting>(*row.map, piece, local);
        row.pin       = &add_window<ui::Sprite>(*row.map, piece, *marker_);
        row.pin->set_visible(false);
        row.frame     = &add_window<ui::Sprite>(*this, piece, frame, offset);
        row.highlight = &add_window<ui::Sprite>(*this, piece, highlight, offset);
        row.built     = &add_window<ui::Sprite>(*this, piece, built, offset);
        connections_.relay(row.picture_window->pressed, row_pressed, rows_.size());
        rows_.push_back(std::move(row));
    }
    set_entries({});
    select(std::nullopt);
}

void TownListView::set_entries(std::span<const TownListEntry> entries) {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        Row& row                   = rows_[i];
        const TownListEntry* entry = i < entries.size() ? &entries[i] : nullptr;
        row.picture_window->set_visible(entry != nullptr);
        row.map->set_visible(entry != nullptr);
        row.built->set_visible(false);
        row.marker = {};
        row.map_image->set_surface(nullptr);
        row.pin->set_visible(false);
        if (entry == nullptr)
            continue;
        compose(*assets_, *entry, *row.picture);
        if (thumbnail_ == nullptr)
            continue;
        const core::Rect box = scene::MiniMap::tile_rect(entry->tiles.left(), entry->tiles.top(), dimension_, thumbnail_size_)
                                   .united(scene::MiniMap::tile_rect(
                                       entry->tiles.right() - 1, entry->tiles.bottom() - 1, dimension_, thumbnail_size_
                                   ));
        const core::Rect mark = marker_->bounds();
        const core::Point pan{mark.centred_in(box).origin() - mark.origin()};
        row.map_image->set_surface(&thumbnail_->image());
        row.pin->set_layer(*marker_, pan);
        row.pin->set_visible(true);
        row.marker = mark.translated(pan);
    }
}

void TownListView::select(std::optional<std::size_t> row) {
    select_row(rows_, row, &Row::frame, &Row::highlight);
}

core::Rect TownListView::row_rect(std::size_t row) const noexcept {
    return row < rows_.size() ? rows_[row].picture_window->rect_in_root() : core::Rect{};
}

}
