#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "h4/formats/adv_object.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"

namespace h4::assets {

template <class T> using Decoded = std::vector<std::pair<std::string, std::unique_ptr<const T>>>;

struct Prefetched {
    Decoded<formats::BitmapGroup> groups;
    Decoded<formats::TerrainTexture> terrains;
    Decoded<formats::AdvObject> objects;
    Decoded<formats::Font> fonts;
    std::optional<std::unique_ptr<const formats::TransitionLibrary>> transitions;
};

}
