#include "h4/model/creature_abilities.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"

namespace h4::model {

namespace {

const std::array<std::vector<std::string_view>, 74> kAbilities{{
    {"flying", "elemental", "insubstantial"},
    {"flying", "resurrection"},
    {"ranged", "mechanical", "long_range", "siege_machine"},
    {"stealth"},
    {"strength"},
    {"ranged", "flying", "random_curse"},
    {"flying", "magic_immunity", "breath_attack"},
    {"flying", "undead", "skeletal", "panic"},
    {"ranged", "normal_melee", "short_range"},
    {"no_retaliation", "3_headed_attack"},
    {"first_strike", "charging"},
    {"ranged", "long_range"},
    {"strikes_twice", "death_protection"},
    {"ranged", "area_effect"},
    {"ranged", "poison"},
    {"teleport", "summons_demons", "life_protection"},
    {"first_strike", "first_strike_immunity", "mechanical"},
    {"magic_resistance"},
    {"elemental", "magic_resistance"},
    {"flying", "fire_shield", "fire_attack", "fire_resistance"},
    {"ranged", "shoots_twice", "ranged_first_strike"},
    {"flying", "spellcaster", "magic_mirror"},
    {"ranged", "elemental", "fire_attack", "fire_resistance"},
    {"flying", "elemental", "stone_skin"},
    {"flying", "spellcaster"},
    {"flying", "undead", "aging", "insubstantial"},
    {"strikes_twice", "berserk"},
    {"mechanical", "magic_resistance"},
    {"flying", "unlimited_retaliation"},
    {"ranged", "giantslayer"},
    {"flying", "no_retaliation", "strike_and_return"},
    {"no_retaliation", "hydra_strike"},
    {"freeze", "cold_attack", "cold_resistance"},
    {"flying", "mana_leech"},
    {"fortune"},
    {"spellcaster"},
    {"flying", "first_strike", "binding"},
    {"ranged", "normal_melee", "unlimited_shots", "stone_gaze"},
    {"hypnotize"},
    {"block"},
    {"ranged", "death_protection"},
    {"undead", "curse"},
    {"no_retaliation"},
    {"terror"},
    {"first_strike"},
    {"bloodlust"},
    {"ranged", "normal_melee", "short_range"},
    {"taxpayer"},
    {"flying", "rebirth", "breath_attack", "fire_resistance"},
    {"long_weapon", "first_strike_immunity"},
    {"sea_bonus"},
    {"mirth"},
    {"devouring"},
    {"undead", "skeletal"},
    {"stunning"},
    {"flying", "no_retaliation"},
    {"flying", "lightning"},
    {"ranged", "normal_melee", "chaos_protection"},
    {"blind"},
    {"regeneration"},
    {"blinding"},
    {"undead", "flying", "no_retaliation", "vampire"},
    {"elemental", "spellcaster", "cold_resistance", "cold_attack"},
    {"first_strike"},
    {"strikes_twice"},
    {"undead", "toughness"},
    {"ranged", "weakness"},
    {"magic_resistance", "stone_skin", "first_strike_immunity"},
    {"teleport", "spellcaster", "magic_mirror"},
    {"ranged", "shoots_twice", "area_effect", "normal_melee"},
    {"charging", "undead", "terror", "regeneration"},
    {"ranged", "mechanical", "no_ranged_penalties", "large_area_effect"},
    {"magic_immunity", "berserk"},
    {"arc_breath_attack", "magic_resistance"},
}};

}

std::span<const std::string_view> creature_abilities(CreatureKindId kind) noexcept {
    const std::size_t id = kind.value();
    return id < kAbilities.size() ? std::span<const std::string_view>{kAbilities[id]} : std::span<const std::string_view>{};
}

bool has_ability(CreatureKindId kind, std::string_view keyword) noexcept {
    return std::ranges::any_of(creature_abilities(kind), [&](std::string_view a) { return core::iequals(a, keyword); });
}

core::Result<AbilityNames> AbilityNames::from_table(const formats::Table& table) {
    AbilityNames out;
    for (std::size_t r = 0; r < table.row_count(); ++r) {
        const std::string_view keyword = table.cell(r, 0), name = table.cell(r, 1);
        if (keyword.empty() || name.empty() || keyword == "Keyword")
            continue;
        out.rows_.push_back({std::string{keyword}, std::string{name}, std::string{table.cell(r, 2)}});
    }
    if (out.rows_.empty())
        return core::fail(core::ErrorCode::corrupt, "table.creature_abilities has no records");
    return out;
}

const AbilityNames::Row* AbilityNames::row(std::string_view keyword) const noexcept {
    const auto it = std::ranges::find_if(rows_, [&](const Row& r) { return core::iequals(r.keyword, keyword); });
    return it != rows_.end() ? &*it : nullptr;
}

std::string_view AbilityNames::name(std::string_view keyword) const noexcept {
    const Row* r = row(keyword);
    return r != nullptr ? std::string_view{r->name} : std::string_view{};
}

std::string_view AbilityNames::help(std::string_view keyword) const noexcept {
    const Row* r = row(keyword);
    return r != nullptr ? std::string_view{r->help} : std::string_view{};
}

}
