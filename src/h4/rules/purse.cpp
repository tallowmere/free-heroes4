#include "h4/rules/purse.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace h4::rules {

namespace {

struct PurseRow {
    std::int32_t gold;
    std::int32_t wood_ore;
    std::int32_t rare;
};

constexpr std::array<PurseRow, 10> kPurses{{
    {20000, 20, 10},
    {15000, 15, 7},
    {10000, 10, 5},
    {5000, 10, 5},
    {0, 10, 5},
    {0, 10, 5},
    {5000, 10, 5},
    {10000, 10, 5},
    {15000, 15, 7},
    {20000, 20, 10},
}};

}

model::ResourceBundle starting_purse(bool computer, model::MapDifficulty difficulty) noexcept {
    const PurseRow& row = kPurses[(computer ? 5U : 0U) + std::to_underlying(difficulty)];
    using model::Resource;
    using model::ResourceAmount;
    return model::ResourceBundle{}
        .with(Resource::gold, ResourceAmount{row.gold})
        .with(Resource::wood, ResourceAmount{row.wood_ore})
        .with(Resource::ore, ResourceAmount{row.wood_ore})
        .with(Resource::crystal, ResourceAmount{row.rare})
        .with(Resource::sulfur, ResourceAmount{row.rare})
        .with(Resource::mercury, ResourceAmount{row.rare})
        .with(Resource::gems, ResourceAmount{row.rare});
}

}
