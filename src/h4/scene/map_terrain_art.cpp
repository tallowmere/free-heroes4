#include "h4/scene/map_terrain_art.hpp"

#include "h4/assets/assets.hpp"
#include "h4/model/terrain_mosaic.hpp"

namespace h4::scene {

const formats::TerrainTexture* TerrainArt::ground(std::uint8_t terrain, std::uint8_t variant, std::uint8_t sub) const {
    if (terrain >= kClasses || variant >= kVariants || sub >= kSubs)
        return assets_->terrain(model::terrain_payload_name(terrain, variant, sub));
    Entry& entry = ground_[(terrain * kVariants + variant) * kSubs + sub];
    if (!entry.resolved)
        entry = {assets_->terrain(model::terrain_payload_name(terrain, variant, sub)), true};
    return entry.art;
}

const formats::TerrainTexture* TerrainArt::road(std::uint8_t road, std::uint8_t sub) const {
    if (road < 1 || road > kRoads || sub >= kSubs)
        return assets_->terrain(model::road_payload_name(road, sub));
    Entry& entry = road_[(road - 1U) * kSubs + sub];
    if (!entry.resolved)
        entry = {assets_->terrain(model::road_payload_name(road, sub)), true};
    return entry.art;
}

}
