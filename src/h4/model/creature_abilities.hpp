#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/creature.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

[[nodiscard]] std::span<const std::string_view> creature_abilities(CreatureKindId kind) noexcept;
[[nodiscard]] bool has_ability(CreatureKindId kind, std::string_view keyword) noexcept;

class AbilityNames {
public:
    [[nodiscard]] static core::Result<AbilityNames> from_table(const formats::Table& table);

    [[nodiscard]] std::string_view name(std::string_view keyword) const noexcept;
    [[nodiscard]] std::string_view help(std::string_view keyword) const noexcept;

private:
    struct Row {
        std::string keyword, name, help;
    };
    [[nodiscard]] const Row* row(std::string_view keyword) const noexcept;

    std::vector<Row> rows_;
};

}
