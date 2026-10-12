#include "h4/app/boot/game_opening.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/model/game_database.hpp"
#include "h4/model/random_passes.hpp"
#include "h4/model/roller.hpp"
#include "h4/model/scenario_resolution.hpp"
#include "h4/model/skills.hpp"
#include "h4/rules/army_movement.hpp"
#include "h4/rules/experience.hpp"
#include "h4/rules/income.hpp"
#include "h4/rules/purse.hpp"
#include "h4/rules/treasure.hpp"

namespace h4::app {

namespace {

std::pair<std::optional<model::Alignment>, std::int32_t> shrine_kind(const model::MapActor& actor) {
    const std::optional<model::Alignment> school = model::school_named_in(actor.model);
    std::int32_t level                           = 0;
    if (const auto at = actor.sub.rfind('_'); at != std::string::npos) {
        const std::string_view digits = std::string_view{actor.sub}.substr(at + 1);
        const auto [end, error]       = std::from_chars(digits.data(), digits.data() + digits.size(), level);
        if (error != std::errc{} || end != digits.data() + digits.size())
            level = 0;
    }
    return {school, level};
}

std::optional<rules::PlayerId> owner_of(const model::ScenarioSetup& setup, const model::MapActor& actor) {
    for (std::size_t row = 0; row < setup.rows().size(); ++row)
        if (actor.owned() && std::to_underlying(setup.rows()[row].colour) == actor.owner)
            return rules::PlayerId{static_cast<std::uint8_t>(row)};
    return std::nullopt;
}

rules::HeroState opening_hero(std::uint32_t actor,
                              std::size_t slot,
                              const formats::HeroRecord& record,
                              const model::GameDatabase& database,
                              model::Roller& roller) {
    const std::int32_t level = model::hero_level(record);
    rules::HeroState hero{actor,
                          static_cast<std::uint8_t>(slot),
                          {},
                          {},
                          rules::experience_for_level(level),
                          level,
                          rules::inventory_of(record, database.artifacts())};
    for (const model::HeroSkill& skill : model::hero_skills(record))
        if (const std::optional<model::Alignment> school = model::magic_school(skill.discipline))
            hero.school_ranks[std::to_underlying(*school)] = skill.rank;
    for (std::size_t id = 0; id < model::kSpellIds; ++id)
        if (record.spells.test(id))
            hero.known.set(id);
    for (const model::Alignment school : model::kAllAlignments)
        if (model::is_magic(school) && hero.school_ranks[std::to_underlying(school)].has_value())
            if (const auto gift = rules::school_gift(school, hero.known, roller))
                hero.known.set(gift->value());
    return hero;
}

std::vector<rules::HeroState> heroes_of(const model::Scenario& scenario, const model::GameDatabase& database, model::Roller& roller) {
    std::vector<rules::HeroState> heroes;
    const auto actors = scenario.map().actors();
    for (std::uint32_t i = 0; i < actors.size(); ++i)
        for (std::size_t slot = 0; slot < actors[i].army.slots.size(); ++slot)
            if (const formats::ArmySlot& member = actors[i].army.slots[slot]; member.kind == formats::ArmySlot::Kind::hero)
                heroes.push_back(opening_hero(i, slot, member.hero, database, roller));
    return heroes;
}

std::vector<rules::ShrineState>
shrines_of(const model::Scenario& scenario, const model::GameDatabase& database, model::Roller& roller) {
    std::vector<rules::ShrineState> shrines;
    rules::SpellUses uses{};
    const auto actors = scenario.map().actors();
    for (std::uint32_t i = 0; i < actors.size(); ++i) {
        if (actors[i].family != "shrine")
            continue;
        const auto [school, level]                = shrine_kind(actors[i]);
        const std::optional<model::SpellId> spell = school ?
            rules::pick_shrine_spell(database.spells(), *school, level, actors[i].spells, uses, roller) :
            std::nullopt;
        const model::Spell* picked                = spell ? database.spells().spell(*spell) : nullptr;
        shrines.push_back({i, spell, picked != nullptr ? picked->school : std::nullopt, picked != nullptr ? picked->level : 0});
    }
    return shrines;
}

std::vector<rules::ArmyPlace>
armies_of(const model::Scenario& scenario, const model::ScenarioSetup& setup, const model::GameDatabase* database) {
    const auto actors = scenario.map().actors();
    std::vector<rules::ArmyPlace> armies;
    for (std::uint32_t i = 0; i < actors.size(); ++i) {
        const model::MapActor& actor = actors[i];
        if (!actor.is_army())
            continue;
        rules::ArmyPlace place{i, owner_of(setup, actor), {actor.position.x, actor.position.y, actor.level, false}, 0, 0};
        if (database != nullptr)
            place.day = rules::army_movement(actor.army, database->creatures()).day;
        armies.push_back(place);
    }
    return armies;
}

std::vector<model::ResourceBundle> purses_of(const model::ScenarioSetup& setup) {
    std::vector<model::ResourceBundle> purses;
    for (const model::SetupRow& row : setup.rows())
        purses.push_back(rules::starting_purse(!row.human, setup.difficulty()));
    return purses;
}

std::vector<rules::Earner> earners_of(const model::Scenario& scenario, const model::ScenarioSetup& setup) {
    const auto actors = scenario.map().actors();
    std::vector<rules::Earner> earners;
    for (std::uint32_t i = 0; i < actors.size(); ++i) {
        const model::MapActor& actor = actors[i];
        rules::Earner earner{i, owner_of(setup, actor), {}};
        if (actor.kind == model::ActorKind::town)
            earner.daily = rules::town_income(actor.town_buildings);
        else if (const auto resource = rules::mine_resource(actor.sub); actor.family == "mine" && resource.has_value())
            earner.daily = rules::mine_income(*resource);
        else
            continue;
        earners.push_back(earner);
    }
    return earners;
}

std::vector<rules::Treasure> treasures_of(const model::Scenario& scenario, const model::GameDatabase* database) {
    const auto actors = scenario.map().actors();
    std::vector<rules::Treasure> treasures;
    model::Roller roller{scenario.map().terrain_seed() ^ 0x7912B0U};
    model::ArtifactPool pool{database != nullptr ? &database->artifacts() : nullptr};
    for (std::uint32_t i = 0; i < actors.size(); ++i)
        if (const auto resource = model::pile_resource(actors[i].sub); actors[i].family == "material_pile" && resource.has_value())
            treasures.push_back({i, rules::roll_pile(*resource, roller), false, false, {}});
        else if (actors[i].family == "treasure") {
            rules::Treasure chest{i, {}, false, actors[i].sub != "sea_chest", {}};
            if (const std::int32_t gold = rules::roll_chest(roller); gold > 0)
                chest.contents = model::ResourceBundle{}.with(model::Resource::gold, model::ResourceAmount{gold});
            else if (const model::ArtifactType* found = pool.pick(roller, model::ArtifactRank::treasure))
                chest.artifact = rules::Artifact{{found->id->value(), -1}, found->worn_slot()};
            else
                continue;
            treasures.push_back(std::move(chest));
        }
    return treasures;
}

std::vector<rules::EventState> events_of(const model::Scenario& scenario) {
    std::vector<rules::EventState> events;
    for (const model::PlacedEvent& event : scenario.placed_events())
        events.push_back({event.gift, event.removes, false});
    return events;
}

rules::GameStart start_of(const model::Scenario& scenario, const model::ScenarioSetup& setup, const model::GameDatabase* database) {
    rules::GameStart start{static_cast<std::uint8_t>(setup.rows().size()),
                           armies_of(scenario, setup, database),
                           purses_of(setup),
                           earners_of(scenario, setup),
                           treasures_of(scenario, database),
                           events_of(scenario),
                           {},
                           {}};
    if (database != nullptr) {
        model::Roller roller{scenario.map().terrain_seed() ^ 0x46B310U};
        start.heroes  = heroes_of(scenario, *database, roller);
        start.shrines = shrines_of(scenario, *database, roller);
    }
    return start;
}

}

core::Result<GameOpening> open_game(model::Scenario scenario, model::ScenarioSetup setup, const model::GameDatabase* database) {
    auto game = rules::GameState::start(start_of(scenario, setup, database));
    if (!game)
        return std::unexpected{game.error()};
    std::optional<rules::PlayerId> me;
    for (std::size_t row = 0; row < setup.rows().size(); ++row)
        if (setup.rows()[row].human) {
            me = rules::PlayerId{static_cast<std::uint8_t>(row)};
            break;
        }
    return GameOpening{std::move(scenario), std::move(setup), std::move(*game), me};
}

core::Result<GameOpening> resolve_and_open(model::Scenario scenario, model::ScenarioSetup setup, const model::GameDatabase* database) {
    model::Scenario resolved = model::resolved_scenario(std::move(scenario), setup, database);
    return open_game(std::move(resolved), std::move(setup), database);
}

}
