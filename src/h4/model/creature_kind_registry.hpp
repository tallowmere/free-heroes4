#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "h4/model/creature.hpp"

namespace h4::model {

inline constexpr std::size_t kCreatureKindCount = 74;

[[nodiscard]] std::optional<CreatureKindId> creature_kind_id(std::string_view keyword) noexcept;
[[nodiscard]] std::string_view creature_kind_keyword(CreatureKindId id) noexcept;
[[nodiscard]] std::span<const std::string_view> creature_kind_keywords() noexcept;

}
