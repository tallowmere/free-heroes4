#pragma once

#include <span>
#include <string_view>

#include "h4/model/creature_kind_registry.hpp"

namespace h4::model {

inline constexpr int kCreatureKinds = static_cast<int>(kCreatureKindCount);

[[nodiscard]] std::string_view creature_actor_keyword(int kind) noexcept;
[[nodiscard]] std::span<const std::string_view> creature_actor_keywords() noexcept;

}
