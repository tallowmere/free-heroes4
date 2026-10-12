#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/hero_class.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

struct HeroRow {
    std::string keyword;
    std::string name;
    int sex = 0;
    std::optional<HeroClassId> hero_class;
    std::optional<int> registry_id;
    bool allowed = false;
    bool generic = false;
};

class HeroTable {
public:
    [[nodiscard]] static core::Result<HeroTable> from_table(const formats::Table& table);

    [[nodiscard]] std::span<const HeroRow> rows() const noexcept {
        return rows_;
    }

private:
    explicit HeroTable(std::vector<HeroRow> rows) noexcept : rows_{std::move(rows)} {
    }

    std::vector<HeroRow> rows_;
};

}
