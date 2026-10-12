#include "h4/formats/table.hpp"

#include <format>

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

core::Result<Table> Table::parse(std::span<const std::byte> data) {
    core::ByteReader reader{data, "table"};

    const std::size_t row_count = reader.bounded_count(reader.u32(), 2);
    std::vector<std::vector<std::string>> rows;
    rows.reserve(row_count);
    for (std::size_t r = 0; r < row_count && reader.ok(); ++r) {
        const std::size_t cells = reader.bounded_count(reader.u16(), 2);
        auto& row               = rows.emplace_back();
        row.reserve(cells);
        for (std::size_t c = 0; c < cells && reader.ok(); ++c)
            row.emplace_back(reader.string16());
    }

    std::vector<std::uint32_t> index;
    if (reader.ok() && !reader.at_end()) {
        const std::size_t count = reader.bounded_count(reader.u32(), 4);
        index.reserve(count);
        for (std::size_t i = 0; i < count && reader.ok(); ++i) {
            const std::uint32_t v = reader.u32();
            if (v >= row_count)
                reader.fail(core::ErrorCode::corrupt, std::format("index entry {} names row {} of {}", i, v, row_count));
            index.push_back(v);
        }
    }
    return reader.finish(Table{std::move(rows), std::move(index)});
}

std::span<const std::string> Table::row(std::size_t r) const noexcept {
    if (r >= rows_.size())
        return {};
    return rows_[r];
}

std::string_view Table::cell(std::size_t r, std::size_t column) const noexcept {
    const auto cells = row(r);
    return column < cells.size() ? std::string_view{cells[column]} : std::string_view{};
}

}
