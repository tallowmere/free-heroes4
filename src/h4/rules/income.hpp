#pragma once

#include <optional>
#include <string_view>

#include "h4/formats/scenario/objects.hpp"
#include "h4/model/resources.hpp"

namespace h4::rules {

[[nodiscard]] model::ResourceBundle town_income(const formats::TownBuildings& buildings) noexcept;

[[nodiscard]] std::optional<model::Resource> mine_resource(std::string_view sub) noexcept;

[[nodiscard]] model::ResourceBundle mine_income(model::Resource resource) noexcept;

}
