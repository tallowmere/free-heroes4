#pragma once

#include <vector>

namespace h4::assets {
struct AssetRequest;
}
namespace h4::model {
class Map;
}

namespace h4::scene {

[[nodiscard]] std::vector<assets::AssetRequest> map_assets(const model::Map& map);

}
