#include "h4/model/random_objects.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <span>
#include <string>
#include <utility>

#include "h4/model/alignment.hpp"
#include "h4/model/fortification.hpp"
#include "h4/model/map.hpp"
#include "h4/model/random_passes.hpp"
#include "h4/model/resources.hpp"

namespace h4::model {

namespace {

constexpr std::array<std::string_view, 6> kCastles{"haven", "academy", "necropolis", "asylum", "preserve", "stronghold"};

[[nodiscard]] std::string_view tier_of(const formats::TownBuildings& buildings) noexcept {
    return to_string(fortification_of(buildings));
}

constexpr std::array<std::string_view, kResourceCount> kPileModels{"Resources.Gold",
                                                                   "Resources.Wood",
                                                                   "Resources.Ore",
                                                                   "Resources.Crystal",
                                                                   "Resources.Sulfur",
                                                                   "Resources.Mercury",
                                                                   "Resources.Gem"};

constexpr std::array<std::string_view, kMagicAlignments> kShrineSchools{"Life", "Order", "Death", "Chaos", "Nature"};
constexpr std::array<std::string_view, kMagicAlignments> kTemples{"moral-luck boosters.Temple of Light",
                                                                  "moral-luck boosters.Temple of Order",
                                                                  "moral-luck boosters.Temple of Darkness",
                                                                  "moral-luck boosters.Temple of Chaos",
                                                                  "moral-luck boosters.Temple of Nature"};
constexpr std::array<std::string_view, 9> kScholarKeywords{"tactician", "trainer", "scout", "noble", "priest", "scholar", "lich", "sorcerer", "druid"};
constexpr std::array<std::string_view, 9> kScholarModels{"Scholars.Tactician",
                                                         "Scholars.Combat",
                                                         "Scholars.Scout",
                                                         "Scholars.Noble",
                                                         "Scholars.Priest",
                                                         "Scholars.Scholar",
                                                         "Scholars.Lich",
                                                         "Scholars.Sorcerer",
                                                         "Scholars.Druid"};
constexpr std::array<std::string_view, 6> kWeeklyModels{"mine.weekly.woodcutters_cottage",
                                                        "mine.weekly.miner's guilds",
                                                        "mine.weekly.crystal pool",
                                                        "mine.weekly.imp pit",
                                                        "mine.weekly.apprentice's lab",
                                                        "mine.weekly.leprechaun"};

[[nodiscard]] std::size_t school_of(const MapActor& actor, const RandomResolution& choices, Roller& roller) {
    const std::optional<Alignment> constraint = alignment_constraint(actor, choices);
    if (constraint.has_value() && is_magic(*constraint))
        return std::to_underlying(*constraint);
    return roller.next_index(kMagicAlignments);
}

[[nodiscard]] std::size_t scholar_kind(const MapActor& actor, const RandomResolution& choices, Roller& roller) {
    const std::optional<Alignment> constraint = alignment_constraint(actor, choices);
    std::uint16_t wanted                      = 0x1FF;
    if (constraint.has_value()) {
        const auto school = std::to_underlying(*constraint);
        wanted = is_magic(*constraint) ? static_cast<std::uint16_t>(1U << (4 + school)) : std::uint16_t{0x00F};
    }
    const std::uint16_t mask = actor.scholar_mask;
    std::uint16_t set        = static_cast<std::uint16_t>(wanted & mask);
    if (set == 0)
        set = mask;
    if (set == 0)
        set = wanted;

    const int count = std::popcount(static_cast<unsigned>(set));
    const std::optional<std::size_t> kind = nth_set_bit(set, roller.next(count));
    return kind.has_value() && *kind < kScholarModels.size() ? *kind : 0;
}

void resolve_small(MapActor& actor, const RandomResolution& choices, Roller& roller) {
    if (actor.family == "random_shrine") {
        const std::size_t school = school_of(actor, choices, roller);
        const int level          = std::max(sub_level(actor.sub), 1);
        actor.model              = "shrines.";
        actor.model += kShrineSchools[school];
        actor.model += ' ';
        actor.model += static_cast<char>('0' + level);
        actor.family = "shrine";
        return;
    }
    if (actor.family == "random_temple") {
        const std::size_t school = school_of(actor, choices, roller);
        actor.model              = std::string{kTemples[school]};
        actor.family             = "temple";
        actor.sub                = std::string{to_string(kAllAlignments[school])};
        return;
    }
    if (actor.family == "random_teacher") {
        const std::size_t kind = scholar_kind(actor, choices, roller);
        actor.model            = std::string{kScholarModels[kind]};
        actor.family           = "teacher";
        actor.sub              = std::string{kScholarKeywords[kind]};
        return;
    }
    if (actor.family == "random_weekly_material_generator") {
        const auto resource = static_cast<std::size_t>(roller.next(6));
        actor.model         = std::string{kWeeklyModels[resource]};
        actor.family        = "weekly_material_generator";
    }
}

}

std::optional<Alignment> alignment_constraint(const MapActor& actor, const RandomResolution& choices) {
    if (actor.constraint.kind != formats::NameSpec::Kind::player)
        return std::nullopt;
    const std::size_t colour = actor.constraint.colour;
    return colour < choices.alignments.size() ? choices.alignments[colour] : std::nullopt;
}

std::optional<Alignment> owner_alignment(const MapActor& actor, const RandomResolution& choices) {
    return actor.owned() && actor.owner < choices.alignments.size() ? choices.alignments[actor.owner] : std::nullopt;
}

int sub_level(std::string_view sub) noexcept {
    for (std::size_t i = 0; i + 1 < sub.size(); ++i)
        if (sub[i] == '_' && sub[i + 1] >= '1' && sub[i + 1] <= '9')
            return sub[i + 1] - '0';
    return 0;
}

void Map::resolve_random_objects(const RandomResolution& choices) {
    Roller stream{seed_};
    resolve_random_objects(choices, stream);
}

void Map::resolve_random_objects(const RandomResolution& choices, Roller& roller) {
    add_starting_troops(choices);
    const std::span<const std::optional<Alignment>> alignments{choices.alignments};

    std::vector<std::optional<Alignment>> settled(actors_.size());
    const auto is_random_town = [](const MapActor& actor) {
        return actor.kind == ActorKind::town && actor.family == "random_town";
    };

    const auto constrained = [&](const MapActor& actor) -> std::optional<Alignment> {
        switch (actor.constraint.kind) {
        case formats::NameSpec::Kind::player: return alignment_constraint(actor, choices);
        case formats::NameSpec::Kind::town:
            for (std::size_t i = 0; i < actors_.size(); ++i)
                if (actors_[i].kind == ActorKind::town && actors_[i].name == actor.constraint.town)
                    return settled[i];
            return std::nullopt;
        case formats::NameSpec::Kind::none: break;
        }
        return std::nullopt;
    };

    for (bool again = true; again;) {
        again = false;
        for (std::size_t i = 0; i < actors_.size(); ++i) {
            if (settled[i].has_value() || !is_random_town(actors_[i]))
                continue;
            const std::optional<Alignment> answer = constrained(actors_[i]);
            if (!answer.has_value())
                continue;
            settled[i] = answer;
            again      = true;
        }
    }

    ArtifactPool artifacts{choices.artifacts};
    HeroPool heroes{choices.heroes, actors_};
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        MapActor& actor = actors_[i];
        if (is_random_town(actor)) {
            if (!settled[i].has_value()) {
                const std::optional<Alignment> owned = owner_alignment(actor, choices);
                settled[i] = owned.has_value() ? *owned : kAllAlignments[roller.next_index(kAllAlignments.size())];
            }
            const std::size_t index = std::to_underlying(*settled[i]);
            actor.model             = "castle.";
            actor.model += kCastles[index];
            actor.model += '.';
            actor.model += tier_of(actor.town_buildings);
            if (actor.sub == "right")
                actor.model += " R";
            actor.family = "town";
            actor.sub    = std::string{to_string(*settled[i])};
            continue;
        }
        if (actor.family == "random_material_pile") {
            const auto resource = static_cast<std::size_t>(roller.next(kResourceCount));
            actor.model         = std::string{kPileModels[resource]};
            actor.family        = "material_pile";
            actor.sub           = std::string{pile_sub(static_cast<Resource>(resource))};
            continue;
        }
        if (actor.family == "random_monster" || actor.family == "random_random_monster") {
            const formats::GridTile* tile = grid_.tile(actor.position.x, actor.position.y, actor.level);
            resolve_random_monster(actor, choices, roller, tile != nullptr && tile->terrain == 0);
            continue;
        }
        if (actor.family == "random_creature_dwelling") {
            resolve_random_dwelling(actor, choices, roller);
            continue;
        }
        resolve_small(actor, choices, roller);
        artifacts.resolve(actor, roller);
        if (actor.kind == ActorKind::hero)
            heroes.resolve(actor, choices, roller);
    }
    add_starting_buildings(roller);
}

}
