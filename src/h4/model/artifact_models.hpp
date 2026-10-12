#pragma once

#include <span>
#include <string_view>

#include "h4/model/artifact_keywords.hpp"

namespace h4::model {

struct ArtifactModel {
    ArtifactId id;
    std::string_view model;
};

[[nodiscard]] std::span<const ArtifactModel> artifact_models() noexcept;

[[nodiscard]] std::string_view artifact_model_name(ArtifactId id) noexcept;

}
