#include "h4/scene/map_assets.hpp"

#include <algorithm>
#include <array>
#include <string>

#include "h4/assets/request.hpp"
#include "h4/model/map.hpp"
#include "h4/model/terrain_mosaic.hpp"
#include "h4/scene/map_frame.hpp"
#include "h4/scene/map_objects_detail.hpp"
#include "h4/scene/map_shroud.hpp"
#include "h4/scene/map_terrain_art.hpp"

namespace h4::scene {

std::vector<assets::AssetRequest> map_assets(const model::Map& map) {
    using assets::AssetRequest;
    std::vector<AssetRequest> out;
    const auto add = [&out](AssetRequest request) {
        if (!request.name.empty())
            out.push_back(std::move(request));
    };
    out.push_back(AssetRequest::transitions());
    out.push_back(AssetRequest::group(std::string{kFrameGroup}));
    out.push_back(AssetRequest::group(std::string{kShroudGroup}));
    add(AssetRequest::object(std::string{detail::kSelectionMarker}));
    for (const model::MapActor& actor : map.actors())
        if (!actor.is_army())
            add(AssetRequest::object(actor.model));

    std::array<bool, TerrainArt::kClasses> classes{};
    std::array<bool, TerrainArt::kRoads + 1> roads{};
    for (const formats::GridTile& tile : map.terrain().tiles()) {
        if (tile.terrain < classes.size())
            classes[tile.terrain] = true;
        if (tile.road < roads.size())
            roads[tile.road] = true;
    }
    for (std::size_t terrain = 0; terrain < classes.size(); ++terrain)
        if (classes[terrain])
            for (std::uint8_t variant = 0; variant < TerrainArt::kVariants; ++variant)
                for (std::uint8_t sub = 0; sub < TerrainArt::kSubs; ++sub)
                    add(AssetRequest::terrain(model::terrain_payload_name(static_cast<std::uint8_t>(terrain), variant, sub)));
    for (std::uint8_t road = 1; road < roads.size(); ++road)
        if (roads[road])
            for (std::uint8_t sub = 0; sub < TerrainArt::kSubs; ++sub)
                add(AssetRequest::terrain(model::road_payload_name(road, sub)));

    std::ranges::sort(out);
    const auto [first, last] = std::ranges::unique(out);
    out.erase(first, last);
    return out;
}

}
