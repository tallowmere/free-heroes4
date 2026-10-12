#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/artifact_keywords.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

enum class ArtifactRank : std::uint8_t { item, treasure, minor, major, relic, fragment, divine };

[[nodiscard]] std::string_view to_string(ArtifactRank rank) noexcept;
[[nodiscard]] std::optional<ArtifactRank> parse_artifact_rank(std::string_view text) noexcept;

inline constexpr std::int32_t kBackpackSlot = 16;

struct ArtifactType {
    std::optional<ArtifactId> id;
    std::string keyword;
    std::string name;
    std::string article;
    std::string file;
    std::string help;
    std::string slot;
    ArtifactRank rank = ArtifactRank::item;
    bool allowed      = false;
    std::string_view model;

    [[nodiscard]] bool potion() const noexcept {
        return slot == "Potion";
    }
    [[nodiscard]] std::int32_t worn_slot() const noexcept;
};

class ArtifactCatalog {
public:
    [[nodiscard]] static core::Result<ArtifactCatalog> from_table(const formats::Table& table);

    [[nodiscard]] std::span<const ArtifactType> all() const noexcept {
        return types_;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return types_.size();
    }

    [[nodiscard]] const ArtifactType* find(ArtifactId id) const noexcept;

private:
    explicit ArtifactCatalog(std::vector<ArtifactType> types);

    std::vector<ArtifactType> types_;
};

}
