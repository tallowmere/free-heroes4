#include "h4/model/hero_class.hpp"

#include <array>
#include <optional>

#include "h4/core/ascii.hpp"
#include "h4/formats/scenario/records.hpp"

namespace h4::model {

namespace {

using D = Discipline;

constexpr std::array<HeroClass, kHeroClasses> kClasses{{
    HeroClass{"knight", Alignment::life, false, {D::tactics, D::defense}},
    HeroClass{"priest", Alignment::life, true, {D::life, D::healing}},
    HeroClass{"lord", Alignment::order, false, {D::nobility, D::estates}},
    HeroClass{"mage", Alignment::order, true, {D::order, D::enchantment}},
    HeroClass{"death_knight", Alignment::death, false, {D::tactics, D::offense}},
    HeroClass{"necromancer", Alignment::death, true, {D::death, D::black}},
    HeroClass{"thief", Alignment::chaos, false, {D::scouting, D::stealth}},
    HeroClass{"sorcerer", Alignment::chaos, true, {D::chaos, D::conjuration}},
    HeroClass{"archer", Alignment::nature, false, {D::toughness, D::archery}},
    HeroClass{"druid", Alignment::nature, true, {D::nature, D::herbalism}},
    HeroClass{"barbarian", Alignment::might, false, {D::toughness, D::combat, D::resistance}},
    HeroClass{"crusader", Alignment::life, false, {D::tactics, D::life}},
    HeroClass{"paladin", Alignment::life, false, {D::toughness, D::life}},
    HeroClass{"prophet", Alignment::life, true, {D::scouting, D::life}},
    HeroClass{"cardinal", Alignment::life, true, {D::nobility, D::life}},
    HeroClass{"monk", Alignment::life, true, {D::life, D::order}},
    HeroClass{"illusionist", Alignment::order, true, {D::tactics, D::order}},
    HeroClass{"battle_mage", Alignment::order, true, {D::toughness, D::order}},
    HeroClass{"seer", Alignment::order, true, {D::scouting, D::order}},
    HeroClass{"wizard_king", Alignment::order, true, {D::nobility, D::order}},
    HeroClass{"wizard", Alignment::order, true, {D::order, D::chaos}},
    HeroClass{"enchanter", Alignment::order, true, {D::order, D::nature}},
    HeroClass{"reaver", Alignment::death, false, {D::tactics, D::death}},
    HeroClass{"assassin", Alignment::death, false, {D::toughness, D::death}},
    HeroClass{"ninja", Alignment::death, false, {D::scouting, D::death}},
    HeroClass{"dark_lord", Alignment::death, false, {D::nobility, D::death}},
    HeroClass{"dark_priest", Alignment::death, true, {D::life, D::death}},
    HeroClass{"shadow_mage", Alignment::death, true, {D::order, D::death}},
    HeroClass{"demonologist", Alignment::death, true, {D::death, D::nature}},
    HeroClass{"pyromancer", Alignment::chaos, true, {D::tactics, D::chaos}},
    HeroClass{"fireguard", Alignment::chaos, false, {D::toughness, D::chaos}},
    HeroClass{"fire_diviner", Alignment::chaos, true, {D::scouting, D::chaos}},
    HeroClass{"witch_king", Alignment::chaos, true, {D::nobility, D::chaos}},
    HeroClass{"heretic", Alignment::chaos, true, {D::life, D::chaos}},
    HeroClass{"lich", Alignment::chaos, true, {D::death, D::chaos}},
    HeroClass{"warlock", Alignment::chaos, true, {D::chaos, D::nature}},
    HeroClass{"warden", Alignment::nature, false, {D::tactics, D::nature}},
    HeroClass{"green_knight", Alignment::nature, false, {D::toughness, D::nature}},
    HeroClass{"bard", Alignment::nature, false, {D::scouting, D::nature}},
    HeroClass{"beast_lord", Alignment::nature, false, {D::nobility, D::nature}},
    HeroClass{"summoner", Alignment::nature, true, {D::life, D::nature}},
    HeroClass{"guildmaster", std::nullopt, false, {D::scouting, D::nobility}},
    HeroClass{"general", std::nullopt, false, {D::tactics, D::toughness}},
    HeroClass{"lord_commander", std::nullopt, false, {D::tactics, D::nobility}},
    HeroClass{"warlord", std::nullopt, false, {D::toughness, D::nobility}},
    HeroClass{"ranger", std::nullopt, false, {D::toughness, D::scouting}},
    HeroClass{"field_marshal", std::nullopt, false, {D::tactics, D::scouting}},
    HeroClass{"archmage", std::nullopt, true, {D::life, D::order, D::death, D::chaos, D::nature}},
}};
static_assert(kHeroClasses == static_cast<std::size_t>(formats::kHeroClasses), "the registry is the table a record's class id indexes");

}

std::span<const HeroClass> hero_classes() noexcept {
    return kClasses;
}

const HeroClass* hero_class(HeroClassId id) noexcept {
    return id.value() < kClasses.size() ? &kClasses[id.value()] : nullptr;
}

std::optional<HeroClassId> hero_class_id(const formats::HeroRecord& hero) noexcept {
    return hero.hero_class >= 0 ? std::optional{HeroClassId{static_cast<std::uint8_t>(hero.hero_class)}} : std::nullopt;
}

const HeroClass* hero_class(const formats::HeroRecord& hero) noexcept {
    const std::optional<HeroClassId> id = hero_class_id(hero);
    return id ? hero_class(*id) : nullptr;
}

std::optional<HeroClassId> hero_class_id(std::string_view keyword) noexcept {
    for (std::size_t i = 0; i < kClasses.size(); ++i)
        if (core::iequals(keyword, kClasses[i].keyword))
            return HeroClassId{static_cast<std::uint8_t>(i)};
    return std::nullopt;
}

std::string hero_actor_name(const formats::HeroRecord& hero) {
    const HeroClass* klass = hero_class(hero);
    if (klass == nullptr || !klass->alignment.has_value())
        return "hero.Random Hero";
    return std::string{"hero."} + std::string{to_string(*klass->alignment)} + "_" + (klass->magic ? "magic" : "might") +
        "_" + (hero.sex == 0 ? "female" : "male");
}

}
