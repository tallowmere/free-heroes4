#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {

class Table {
public:
    [[nodiscard]] static core::Result<Table> parse(std::span<const std::byte> data);

    [[nodiscard]] std::size_t row_count() const noexcept {
        return rows_.size();
    }

    [[nodiscard]] std::span<const std::string> row(std::size_t row) const noexcept;

    [[nodiscard]] std::string_view cell(std::size_t row, std::size_t column) const noexcept;

    [[nodiscard]] std::span<const std::uint32_t> row_index() const noexcept {
        return index_;
    }

private:
    Table(std::vector<std::vector<std::string>> rows, std::vector<std::uint32_t> index)
    : rows_{std::move(rows)}, index_{std::move(index)} {
    }

    std::vector<std::vector<std::string>> rows_;
    std::vector<std::uint32_t> index_;
};

}
