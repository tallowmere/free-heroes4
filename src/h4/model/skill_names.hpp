#pragma once

#include <array>
#include <string>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/model/skills.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

class SkillNames {
public:
    [[nodiscard]] static core::Result<SkillNames> from_table(const formats::Table& table);

    [[nodiscard]] std::string_view name(Discipline discipline, int rank) const noexcept;
    [[nodiscard]] std::string_view help(Discipline discipline, int rank) const noexcept;

private:
    static constexpr std::size_t kEntries = kDisciplines * kRanks;
    struct Entry {
        std::string name, help;
    };
    [[nodiscard]] static std::size_t slot(Discipline discipline, int rank) noexcept;
    [[nodiscard]] const Entry* entry(Discipline discipline, int rank) const noexcept;

    std::array<Entry, kEntries> entries_{};
};

}
