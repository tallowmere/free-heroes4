#include "h4/model/creature_catalog.hpp"

#include <algorithm>
#include <format>

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"
#include "h4/model/cell_parsing.hpp"
#include "h4/model/creature_kind_registry.hpp"

namespace h4::model {

namespace {

enum Column : std::size_t {
    keyword       = 0,
    name          = 1,
    plural        = 3,
    level         = 5,
    alignment     = 6,
    hit_points    = 7,
    damage_low    = 8,
    damage_high   = 9,
    attack        = 10,
    defense       = 11,
    movement      = 12,
    speed         = 13,
    shots         = 14,
    spell_points  = 15,
    weekly_growth = 17,
    cost_first    = 18,
    experience    = 25,
    long_help     = 27,
};

}

CreatureCatalog::CreatureCatalog(std::vector<CreatureType> types)
: types_{std::move(types)}, by_id_(kCreatureKindCount) {
    for (std::size_t i = 0; i < types_.size(); ++i)
        by_id_[types_[i].id().value()] = i;
}

core::Result<CreatureCatalog> CreatureCatalog::from_table(const formats::Table& table) {
    std::vector<CreatureType> types;
    for (std::size_t r = 0; r < table.row_count(); ++r) {
        const auto cell = [&](std::size_t c) { return table.cell(r, c); };

        const auto level     = parse_int(cell(Column::level));
        const auto alignment = parse_alignment(cell(Column::alignment));
        if (!level || !alignment || *level < 1 || *level > 4)
            continue;

        const std::string_view key = cell(Column::keyword);
        const auto id              = creature_kind_id(key);
        if (!id)
            return core::fail(core::ErrorCode::corrupt, std::format("creature row {}: unknown keyword '{}'", r, key));

        ResourceBundle cost;
        for (std::size_t i = 0; i < kResourceCount; ++i)
            cost = cost.with(static_cast<Resource>(i), ResourceAmount{parse_int_or_zero(cell(Column::cost_first + i))});

        CreatureStats stats{
            .level        = CreatureLevel{static_cast<std::uint8_t>(*level)},
            .hit_points   = HitPoints{parse_int_or_zero(cell(Column::hit_points))},
            .damage       = {parse_int_or_zero(cell(Column::damage_low)), parse_int_or_zero(cell(Column::damage_high))},
            .attack       = parse_int_or_zero(cell(Column::attack)),
            .defense      = parse_int_or_zero(cell(Column::defense)),
            .movement     = parse_int_or_zero(cell(Column::movement)),
            .speed        = parse_int_or_zero(cell(Column::speed)),
            .shots        = parse_int_or_zero(cell(Column::shots)),
            .spell_points = parse_int_or_zero(cell(Column::spell_points)),
            .weekly_growth = parse_int_or_zero(cell(Column::weekly_growth)),
            .experience    = Experience{parse_int_or_zero(cell(Column::experience))},
        };
        types.emplace_back(*id,
                           std::string{key},
                           CreatureNames{std::string{cell(Column::name)},
                                         std::string{cell(Column::plural)},
                                         std::string{cell(Column::long_help)}},
                           *alignment,
                           stats,
                           cost);
    }

    if (types.empty())
        return core::fail(core::ErrorCode::corrupt, "table.creatures has no rows");

    for (std::size_t i = 0; i < types.size(); ++i)
        for (std::size_t j = i + 1; j < types.size(); ++j)
            if (types[i].id() == types[j].id())
                return core::fail(core::ErrorCode::corrupt, std::format("creature '{}' appears twice", types[i].keyword()));
    return CreatureCatalog{std::move(types)};
}

const CreatureType* CreatureCatalog::find(CreatureKindId id) const noexcept {
    if (id.value() >= by_id_.size() || !by_id_[id.value()])
        return nullptr;
    return &types_[*by_id_[id.value()]];
}

const CreatureType* CreatureCatalog::find(std::string_view keyword) const noexcept {
    const auto id = creature_kind_id(keyword);
    return id ? find(*id) : nullptr;
}

}
