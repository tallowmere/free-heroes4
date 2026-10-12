#include "h4/model/skill_names.hpp"

#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"
#include "h4/model/skills.hpp"

namespace h4::model {

namespace {

constexpr std::size_t kKeyword = 0, kName = 1, kHelp = 2;

}

core::Result<SkillNames> SkillNames::from_table(const formats::Table& table) {
    SkillNames out;
    std::size_t found = 0;
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const std::string_view keyword = table.cell(row, kKeyword);
        for (const Discipline d : kAllDisciplines)
            for (int r = 0; r < kRanks; ++r) {
                const std::string key = std::string{discipline_keyword(d)} + "_" + std::string{rank_keyword(r)};
                if (!core::iequals(key, keyword))
                    continue;
                Entry& e = out.entries_[slot(d, r)];
                e.name   = std::string{table.cell(row, kName)};
                e.help   = std::string{table.cell(row, kHelp)};
                ++found;
            }
    }
    if (found == 0)
        return core::fail(core::ErrorCode::corrupt, "table.skills names no skill");
    return out;
}

std::size_t SkillNames::slot(Discipline discipline, int rank) noexcept {
    return std::to_underlying(discipline) * std::size_t{kRanks} + static_cast<std::size_t>(rank);
}

const SkillNames::Entry* SkillNames::entry(Discipline discipline, int rank) const noexcept {
    return rank >= 0 && rank < kRanks ? &entries_[slot(discipline, rank)] : nullptr;
}

std::string_view SkillNames::name(Discipline discipline, int rank) const noexcept {
    const Entry* e = entry(discipline, rank);
    return e != nullptr ? std::string_view{e->name} : std::string_view{};
}

std::string_view SkillNames::help(Discipline discipline, int rank) const noexcept {
    const Entry* e = entry(discipline, rank);
    return e != nullptr ? std::string_view{e->help} : std::string_view{};
}

}
