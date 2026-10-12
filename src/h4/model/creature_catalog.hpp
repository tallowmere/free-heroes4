#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/creature.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

class CreatureCatalog {
public:
    [[nodiscard]] static core::Result<CreatureCatalog> from_table(const formats::Table& table);

    [[nodiscard]] std::span<const CreatureType> all() const noexcept {
        return types_;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return types_.size();
    }

    [[nodiscard]] const CreatureType* find(CreatureKindId id) const noexcept;
    [[nodiscard]] const CreatureType* find(std::string_view keyword) const noexcept;

private:
    explicit CreatureCatalog(std::vector<CreatureType> types);

    std::vector<CreatureType> types_;
    std::vector<std::optional<std::size_t>> by_id_;
};

}
