#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

class AdventureTexts {
public:
    [[nodiscard]] static core::Result<AdventureTexts> from_table(const formats::Table& table);

    [[nodiscard]] std::string_view
    text(std::string_view major, std::string_view minor, std::string_view subtype, std::string_view keyword) const noexcept;

private:
    struct Row {
        std::string major, minor, subtype, keyword, text;
    };
    explicit AdventureTexts(std::vector<Row> rows) noexcept : rows_{std::move(rows)} {
    }

    std::vector<Row> rows_;
};

[[nodiscard]] std::string
substituted(std::string_view pattern, std::initializer_list<std::pair<std::string_view, std::string>> keys);
[[nodiscard]] std::string
substituted_in_order(std::string_view pattern, std::string_view token, std::initializer_list<std::string> values);

}
