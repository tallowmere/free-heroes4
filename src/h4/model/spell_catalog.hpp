#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/strong.hpp"
#include "h4/model/alignment.hpp"

namespace h4::formats {
class Table;
}

namespace h4::model {

inline constexpr std::size_t kSpellIds = 188;
using SpellId = core::Strong<struct SpellIdTag, std::uint8_t>;

struct Spell {
    SpellId id;
    std::string keyword;
    std::string name;
    std::int32_t level = 0;
    std::optional<Alignment> school;
    bool teachable = false;
};

class SpellCatalog {
public:
    [[nodiscard]] static core::Result<SpellCatalog> from_table(const formats::Table& table);

    [[nodiscard]] const Spell* spell(SpellId id) const noexcept;
    [[nodiscard]] std::span<const Spell> all() const noexcept {
        return spells_;
    }

private:
    explicit SpellCatalog(std::vector<Spell> spells) noexcept : spells_{std::move(spells)} {
    }

    std::vector<Spell> spells_;
};

[[nodiscard]] std::string_view school_keyword(Alignment school) noexcept;
[[nodiscard]] std::string school_word(Alignment school);
[[nodiscard]] std::optional<Alignment> school_named_in(std::string_view model);

}
