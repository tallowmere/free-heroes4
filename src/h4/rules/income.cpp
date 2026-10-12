#include "h4/rules/income.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace h4::rules {

namespace {

constexpr std::uint8_t kTownHall = 1;
constexpr std::uint8_t kCityHall = 2;
constexpr std::uint8_t kGrail    = 11;

constexpr std::array<std::int32_t, model::kResourceCount> kMineYield{1000, 2, 2, 1, 1, 1, 1};

constexpr std::array<std::pair<std::string_view, model::Resource>, 7> kMines{{
    {"gold_mine", model::Resource::gold},
    {"sawmill", model::Resource::wood},
    {"ore_pit", model::Resource::ore},
    {"crystal_mine", model::Resource::crystal},
    {"sulfur_mine", model::Resource::sulfur},
    {"alchemists_lab", model::Resource::mercury},
    {"gem_mine", model::Resource::gems},
}};

}

model::ResourceBundle town_income(const formats::TownBuildings& buildings) noexcept {
    std::int32_t gold = buildings.has(kCityHall) ? 1000 : 500 + (buildings.has(kTownHall) ? 250 : 0);
    if (buildings.has(kGrail))
        gold += 3000;
    return model::ResourceBundle{}.with(model::Resource::gold, model::ResourceAmount{gold});
}

std::optional<model::Resource> mine_resource(std::string_view sub) noexcept {
    for (const auto& [name, resource] : kMines)
        if (name == sub)
            return resource;
    return std::nullopt;
}

model::ResourceBundle mine_income(model::Resource resource) noexcept {
    return model::ResourceBundle{}.with(resource, model::ResourceAmount{kMineYield[std::to_underlying(resource)]});
}

}
