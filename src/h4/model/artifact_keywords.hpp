#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "h4/core/strong.hpp"

namespace h4::model {

inline constexpr int kArtifactIds = 248;

using ArtifactId = core::Strong<struct ArtifactIdTag, std::uint8_t>;

[[nodiscard]] constexpr std::optional<ArtifactId> artifact_id_of(std::int32_t raw) noexcept {
    if (raw < 0 || raw >= kArtifactIds)
        return std::nullopt;
    return ArtifactId{static_cast<std::uint8_t>(raw)};
}

[[nodiscard]] std::optional<ArtifactId> artifact_id(std::string_view keyword) noexcept;

[[nodiscard]] std::string_view artifact_keyword(ArtifactId id) noexcept;

}
