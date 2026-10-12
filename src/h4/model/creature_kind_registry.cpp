#include "h4/model/creature_kind_registry.hpp"

#include <array>

#include "h4/core/ascii.hpp"
#include "h4/formats/scenario/records.hpp"

namespace h4::model {

static_assert(kCreatureKindCount == formats::kCreatureCount, "the registry is the table a record's creature id indexes");

namespace {

constexpr std::array<std::string_view, kCreatureKindCount> kKeywords{
    "air elemental",
    "archangel",
    "ballista",
    "bandit",
    "behemoth",
    "beholder",
    "black dragon",
    "bone dragon",
    "centaur",
    "cerberus",
    "champion",
    "crossbowman",
    "crusader",
    "cyclops",
    "venom spawn",
    "archdevil",
    "dragon golem",
    "dwarf",
    "earth elemental",
    "efreet",
    "elf",
    "faerie dragon",
    "fire elemental",
    "gargoyle",
    "genie",
    "ghost",
    "berserker",
    "gold golem",
    "griffin",
    "halfling",
    "harpy",
    "hydra",
    "ice demon",
    "imp",
    "leprechaun",
    "mage",
    "mantis",
    "medusa",
    "mermaid",
    "minotaur",
    "monk",
    "mummy",
    "naga",
    "nightmare",
    "nomad",
    "ogre mage",
    "orc",
    "peasant",
    "phoenix",
    "pikeman",
    "pirate",
    "satyr",
    "sea monster",
    "skeleton",
    "squire",
    "sprite",
    "thunderbird",
    "titan",
    "troglodyte",
    "troll",
    "unicorn",
    "vampire",
    "water elemental",
    "white tiger",
    "wolf",
    "zombie",
    "waspwort",
    "goblin knight",
    "evil sorceress",
    "gargantuan",
    "dark champion",
    "catapult",
    "frenzied gnasher",
    "mega dragon",
};

}

std::optional<CreatureKindId> creature_kind_id(std::string_view keyword) noexcept {
    const auto i = core::index_of(kKeywords, keyword);
    return i.has_value() ? std::optional{CreatureKindId{static_cast<std::uint8_t>(*i)}} : std::nullopt;
}

std::string_view creature_kind_keyword(CreatureKindId id) noexcept {
    return id.value() < kKeywords.size() ? kKeywords[id.value()] : std::string_view{};
}

std::span<const std::string_view> creature_kind_keywords() noexcept {
    return kKeywords;
}

}
