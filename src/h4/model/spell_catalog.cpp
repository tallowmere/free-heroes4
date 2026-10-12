#include "h4/model/spell_catalog.hpp"

#include <algorithm>
#include <array>
#include <cctype>

#include "h4/core/ascii.hpp"
#include "h4/formats/table.hpp"
#include "h4/model/alignment.hpp"
#include "h4/model/cell_parsing.hpp"

namespace h4::model {

namespace {

constexpr std::array<std::string_view, kSpellIds> kKeywords{
    "acid",
    "aging",
    "animate dead",
    "anti magic",
    "armageddon",
    "banish",
    "berserk",
    "bind flyer",
    "bind wound",
    "binding",
    "bless",
    "blind",
    "bloodfrenzy",
    "bloodlust",
    "burning hands",
    "cancellation",
    "celestial armor",
    "chain lightning",
    "clone",
    "cloud of confusion",
    "cold resistance",
    "confusion",
    "cowardice",
    "curse",
    "death call",
    "despair",
    "disintegrate",
    "dispel",
    "displacement",
    "divine aura",
    "divine intervention",
    "divine protection",
    "divine retribution",
    "dragon strength",
    "evil hour",
    "exorcism",
    "fear",
    "fear_effect",
    "fire_aura_effect",
    "fire bolt",
    "fire resistance",
    "fire ring",
    "fire shield",
    "fireball",
    "firebreathing",
    "first strike",
    "flight",
    "forgetfulness",
    "fortune",
    "freeze",
    "choking gas",
    "giant strength",
    "greater forgetfulness",
    "guardian angel",
    "hand of death",
    "haste",
    "heal",
    "health boost",
    "holy armor",
    "holy retribution",
    "holy water",
    "holy word",
    "hypnotize",
    "ice bolt",
    "illusionary ally",
    "implosion",
    "inferno",
    "kreegan fire",
    "life drain",
    "lightning",
    "luck",
    "magic arrow",
    "magic fist",
    "magic leech",
    "magic mirror",
    "magic resistance",
    "mana",
    "mana flare",
    "mana_leech",
    "martyr",
    "mass bless",
    "mass cancellation",
    "mass curse",
    "mass dispel",
    "mass exorcism",
    "mass first strike",
    "mass fortune",
    "mass haste",
    "mass healing",
    "mass misfortune",
    "mass precision",
    "mass protection from chaos",
    "mass protection from death",
    "mass protection from nature",
    "mass protection from order",
    "mass slayer",
    "mass slow",
    "mass snake strike",
    "mass sorrow",
    "mass spiritual fervor",
    "mass weakness",
    "mire",
    "misfortune",
    "morale",
    "nature haste",
    "order slow",
    "pain mirror",
    "panic",
    "pathfinding",
    "blur",
    "plague",
    "poison",
    "poison attack",
    "power drain",
    "prayer",
    "precision",
    "immortality",
    "protection from chaos",
    "protection from death",
    "protection from life",
    "protection from nature",
    "protection from order",
    "quickness",
    "quicksand",
    "raise ghost",
    "raise skeletons",
    "raise vampires",
    "mass_blur",
    "ray of suffering",
    "rebirth",
    "reflexes",
    "regeneration",
    "endurance",
    "resurrection",
    "rock growth",
    "sacred ground",
    "sacrifice",
    "sanctuary",
    "short hypnotism",
    "simple healing",
    "slayer",
    "slow",
    "smoke",
    "snake strike",
    "song of peace",
    "sorrow",
    "speed",
    "spell shackle",
    "spiritual fervor",
    "steal all enchantments",
    "steal enchantment",
    "stoneskin",
    "stone_gaze",
    "strength",
    "stun",
    "summon elf",
    "summon tiger",
    "summon air elemental",
    "summon boat",
    "summon cerberus",
    "summon demon",
    "summon venom spawn",
    "summon devils",
    "summon earth elemental",
    "summon faerie dragon",
    "summon fire elemental",
    "summon griffin",
    "summon ice demon",
    "summon imps",
    "summon leprechaun",
    "summon mantis",
    "summon phoenix",
    "summon satyr",
    "summon sprite",
    "summon stinkbug",
    "summon unicorn",
    "summon water elemental",
    "summon wolf",
    "teleport",
    "terrain walk",
    "terror",
    "town gate",
    "vampiric touch",
    "visions",
    "water walk",
    "weakness",
    "frozen",
    "summon_waspwort",
};

constexpr std::size_t kKeyword   = 0;
constexpr std::size_t kName      = 1;
constexpr std::size_t kLevel     = 3;
constexpr std::size_t kAlignment = 5;
constexpr std::size_t kTeachable = 32;
constexpr std::size_t kFirstRecord = 2;

}

std::string_view school_keyword(Alignment school) noexcept {
    return is_magic(school) ? to_string(school) : std::string_view{};
}

std::string school_word(Alignment school) {
    std::string out{school_keyword(school)};
    if (!out.empty())
        out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    return out;
}

std::optional<Alignment> school_named_in(std::string_view model) {
    std::optional<Alignment> school;
    for (const Alignment s : kAllAlignments)
        if (is_magic(s) && core::icontains(model, school_word(s)))
            school = s;
    return school;
}

core::Result<SpellCatalog> SpellCatalog::from_table(const formats::Table& table) {
    std::array<std::optional<Spell>, kSpellIds> by_id;
    for (std::size_t row = kFirstRecord; row < table.row_count(); ++row) {
        const std::string_view keyword = table.cell(row, kKeyword);
        if (keyword.empty())
            continue;
        const auto found = core::index_of(kKeywords, keyword);
        if (!found.has_value())
            continue;
        Spell& spell                 = by_id[*found].emplace();
        spell.id                     = SpellId{static_cast<std::uint8_t>(*found)};
        spell.keyword                = std::string{kKeywords[*found]};
        spell.name                   = std::string{table.cell(row, kName)};
        const std::string_view level = table.cell(row, kLevel);
        spell.level                  = parse_int_or_zero(level);
        if (const auto alignment = parse_alignment(table.cell(row, kAlignment)); alignment.has_value() && is_magic(*alignment))
            spell.school = *alignment;
        spell.teachable = !table.cell(row, kTeachable).empty();
    }
    std::vector<Spell> spells;
    for (std::optional<Spell>& spell : by_id)
        if (spell.has_value())
            spells.push_back(std::move(*spell));
    if (spells.empty())
        return core::fail(core::ErrorCode::corrupt, "table.Spells names no spell");
    return SpellCatalog{std::move(spells)};
}

const Spell* SpellCatalog::spell(SpellId id) const noexcept {
    const auto found = std::ranges::lower_bound(spells_, id, {}, &Spell::id);
    return found != spells_.end() && found->id == id ? &*found : nullptr;
}

}
