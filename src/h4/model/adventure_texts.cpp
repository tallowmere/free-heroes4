#include "h4/model/adventure_texts.hpp"

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"

namespace h4::model {

core::Result<AdventureTexts> AdventureTexts::from_table(const formats::Table& table) {
    std::vector<Row> rows;
    for (std::size_t row = 1; row < table.row_count(); ++row) {
        if (table.cell(row, 0).empty() || table.cell(row, 3).empty())
            continue;
        rows.push_back({std::string{table.cell(row, 0)},
                        std::string{table.cell(row, 1)},
                        std::string{table.cell(row, 2)},
                        std::string{table.cell(row, 3)},
                        std::string{table.cell(row, 4)}});
    }
    if (rows.empty())
        return core::fail(core::ErrorCode::corrupt, "table.Adventure Object has no rows");
    return AdventureTexts{std::move(rows)};
}

std::string_view
AdventureTexts::text(std::string_view major, std::string_view minor, std::string_view subtype, std::string_view keyword) const noexcept {
    const auto find = [&](std::string_view m, std::string_view n, std::string_view s) -> const Row* {
        for (const Row& row : rows_)
            if (core::iequals(row.major, m) && core::iequals(row.minor, n) && core::iequals(row.subtype, s) &&
                core::iequals(row.keyword, keyword))
                return &row;
        return nullptr;
    };
    if (!subtype.empty())
        if (const Row* row = find(major, minor, subtype))
            return row->text;
    if (!minor.empty())
        if (const Row* row = find(major, minor, {}))
            return row->text;
    if (const Row* row = find(major, {}, {}))
        return row->text;
    return {};
}

std::string substituted(std::string_view pattern, std::initializer_list<std::pair<std::string_view, std::string>> keys) {
    std::string out{pattern};
    for (const auto& [key, value] : keys)
        for (std::size_t at = out.find(key); at != std::string::npos; at = out.find(key, at + value.size()))
            out.replace(at, key.size(), value);
    return out;
}

std::string substituted_in_order(std::string_view pattern, std::string_view token, std::initializer_list<std::string> values) {
    std::string out{pattern};
    std::size_t at = 0;
    for (const std::string& value : values) {
        at = out.find(token, at);
        if (at == std::string::npos)
            break;
        out.replace(at, token.size(), value);
        at += value.size();
    }
    return out;
}

}
