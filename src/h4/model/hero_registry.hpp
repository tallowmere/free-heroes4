#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "h4/model/alignment.hpp"

namespace h4::model {

struct HeroEntry {
    Alignment alignment = Alignment::life;
    std::string_view keyword;
};

[[nodiscard]] std::span<const HeroEntry> hero_registry() noexcept;
[[nodiscard]] const HeroEntry* hero_entry(int id) noexcept;
[[nodiscard]] std::optional<int> hero_id(std::string_view keyword) noexcept;

}
