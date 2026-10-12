#include <format>
#include <optional>
#include <string>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/core/ascii.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/scenario/records.hpp"
#include "h4/model/artifact_catalog.hpp"
#include "h4/model/creature_kind_registry.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/hero_registry.hpp"
#include "h4/model/skills.hpp"

namespace h4::app {

const formats::Layer* hero_face(const assets::Assets& assets, const formats::HeroRecord& hero, int size) {
    const model::HeroEntry* entry = model::hero_entry(hero.id);
    if (entry == nullptr)
        return nullptr;
    const formats::BitmapGroup* group = assets.group(std::format("icons.hero.{}.{}", model::to_string(entry->alignment), size));
    return group != nullptr ? group->find(entry->keyword) : nullptr;
}

const formats::Layer* creature_face(const assets::Assets& assets, std::int16_t creature, int size) {
    const std::optional<model::CreatureKindId> kind = model::creature_kind_of(creature);
    if (!kind)
        return nullptr;
    const formats::BitmapGroup* group = assets.group(std::format("icons.creatures.{}", size));
    if (group == nullptr)
        return nullptr;
    return group->find(model::creature_kind_keyword(*kind));
}

const formats::Layer* artifact_icon(const assets::Assets& assets, const model::ArtifactType& type) {
    const formats::BitmapGroup* group = assets.group("icons.artifacts." + core::ascii_lowered(type.file));
    return group != nullptr ? group->find(type.keyword) : nullptr;
}

screens::NamedIcon skill_icon(const assets::Assets& assets, const model::GameDatabase* database, const model::HeroSkill& skill) {
    screens::NamedIcon icon;
    if (const formats::BitmapGroup* group = assets.group(std::format("icons.skills.{}.52", model::school_icon_name(skill.discipline)))) {
        icon.base    = group->find(model::discipline_keyword(skill.discipline));
        icon.overlay = group->find(model::rank_keyword(skill.rank));
    }
    if (database != nullptr)
        icon.help = {std::string{database->skills().name(skill.discipline, skill.rank)},
                     std::string{database->skills().help(skill.discipline, skill.rank)}};
    return icon;
}

screens::NamedIcon ability_icon(const assets::Assets& assets, const model::GameDatabase& database, std::string_view ability) {
    screens::NamedIcon icon;
    if (const formats::BitmapGroup* group = assets.group("icons.skills.creature.52"))
        icon.base = group->find(ability);
    icon.help = {std::string{database.abilities().name(ability)}, std::string{database.abilities().help(ability)}};
    return icon;
}

}
