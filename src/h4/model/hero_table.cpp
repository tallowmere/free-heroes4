#include "h4/model/hero_table.hpp"

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"
#include "h4/model/hero_class.hpp"
#include "h4/model/hero_registry.hpp"

namespace h4::model {

namespace {

constexpr std::size_t kKeyword = 0;
constexpr std::size_t kName    = 1;
constexpr std::size_t kSex     = 2;
constexpr std::size_t kClass   = 3;
constexpr std::size_t kAllowed = 5;

}

core::Result<HeroTable> HeroTable::from_table(const formats::Table& table) {
    std::vector<HeroRow> rows;
    rows.reserve(table.row_count());
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const std::string_view keyword = table.cell(row, kKeyword);
        const std::string_view sex     = table.cell(row, kSex);
        const bool male = core::iequals(sex, "male");
        if (keyword.empty() || (!male && !core::iequals(sex, "female")))
            continue;

        HeroRow out;
        out.keyword = std::string{keyword};
        out.name    = std::string{table.cell(row, kName)};
        out.sex     = male ? 1 : 0;
        out.generic = core::iequals(keyword, "generic");
        if (!out.generic)
            out.hero_class = hero_class_id(table.cell(row, kClass));
        out.allowed = !table.cell(row, kAllowed).empty();
        if (!out.generic)
            if (const std::optional<int> id = hero_id(keyword); id.has_value())
                out.registry_id = *id;
        rows.push_back(std::move(out));
    }
    if (rows.empty())
        return core::fail(core::ErrorCode::corrupt, "table.heroes has no rows");
    return HeroTable{std::move(rows)};
}

}
