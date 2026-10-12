#include "h4/model/artifact_catalog.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"
#include "h4/model/artifact_keywords.hpp"
#include "h4/model/artifact_models.hpp"

namespace h4::model {

namespace {

constexpr std::size_t kKeyword = 0, kFile = 1, kName = 2, kArticle = 3, kSlot = 4, kLevel = 6, kHelp = 7, kAllowed = 10;

constexpr std::array<std::string_view, 7> kRankNames{"item", "treasure", "minor", "major", "relic", "fragment", "divine"};

}

std::int32_t ArtifactType::worn_slot() const noexcept {
    constexpr std::array<std::pair<std::string_view, std::int32_t>, 13> kSlots{{
        {"Bow", 0},
        {"Feet", 1},
        {"Head", 2},
        {"Neck", 8},
        {"Left Hand", 10},
        {"Shoulders", 11},
        {"Torso", 12},
        {"Right Hand", 13},
        {"Both Hands", 15},
        {"Backpack", 16},
        {"Potion", 17},
        {"Miscellaneous", 18},
        {"Ring", 19},
    }};
    for (const auto& [label, worn] : kSlots)
        if (core::iequals(label, slot))
            return worn;
    return kBackpackSlot;
}

std::string_view to_string(ArtifactRank rank) noexcept {
    const auto index = std::to_underlying(rank);
    return index < kRankNames.size() ? kRankNames[index] : std::string_view{};
}

std::optional<ArtifactRank> parse_artifact_rank(std::string_view text) noexcept {
    const auto i = core::index_of(kRankNames, text);
    return i.has_value() ? std::optional{static_cast<ArtifactRank>(*i)} : std::nullopt;
}

ArtifactCatalog::ArtifactCatalog(std::vector<ArtifactType> types) : types_{std::move(types)} {
}

const ArtifactType* ArtifactCatalog::find(ArtifactId id) const noexcept {
    for (const ArtifactType& type : types_)
        if (type.id == id)
            return &type;
    return nullptr;
}

core::Result<ArtifactCatalog> ArtifactCatalog::from_table(const formats::Table& table) {
    std::vector<ArtifactType> types;
    types.reserve(table.row_count());
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const std::string_view keyword = table.cell(row, kKeyword);
        const std::optional<ArtifactRank> rank = parse_artifact_rank(table.cell(row, kLevel));
        if (keyword.empty() || !rank.has_value())
            continue;

        ArtifactType type;
        type.keyword = std::string{keyword};
        type.name    = std::string{table.cell(row, kName)};
        type.article = std::string{table.cell(row, kArticle)};
        type.file    = std::string{table.cell(row, kFile)};
        type.help    = std::string{table.cell(row, kHelp)};
        type.slot    = std::string{table.cell(row, kSlot)};
        type.rank    = *rank;
        type.allowed = !table.cell(row, kAllowed).empty();
        types.push_back(std::move(type));
    }
    if (types.empty())
        return core::fail(core::ErrorCode::corrupt, "table.Artifacts has no rows");

    for (ArtifactType& type : types)
        if (const std::optional<ArtifactId> id = artifact_id(type.keyword)) {
            type.id    = *id;
            type.model = artifact_model_name(*id);
        }
    return ArtifactCatalog{std::move(types)};
}

}
