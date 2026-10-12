#include "h4/formats/scenario/families.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "h4/core/ascii.hpp"

namespace h4::formats {

namespace {

using Row = std::pair<std::string_view, ObjectBody>;

constexpr std::array kFamilies{
    Row{"abandoned_mine", ObjectBody::abandoned_mine},
    Row{"academy", ObjectBody::stationary},
    Row{"arena", ObjectBody::stationary},
    Row{"artifact", ObjectBody::stationary},
    Row{"blacksmith", ObjectBody::stationary},
    Row{"blattner_stone", ObjectBody::stationary},
    Row{"border_gate", ObjectBody::stationary},
    Row{"border_guard", ObjectBody::stationary},
    Row{"bridge", ObjectBody::stationary},
    Row{"buoy", ObjectBody::stationary},
    Row{"campfire", ObjectBody::stationary},
    Row{"carryover_hero", ObjectBody::carryover_hero},
    Row{"cartographer", ObjectBody::stationary},
    Row{"chain", ObjectBody::stationary},
    Row{"clover_field", ObjectBody::stationary},
    Row{"coliseum", ObjectBody::stationary},
    Row{"conservatory", ObjectBody::conservatory},
    Row{"corpse", ObjectBody::stationary},
    Row{"creature_bank", ObjectBody::stationary},
    Row{"creature_dwelling", ObjectBody::weekly_generator},
    Row{"decorative", ObjectBody::stationary},
    Row{"deletion_marker", ObjectBody::named},
    Row{"dolphin_school", ObjectBody::stationary},
    Row{"event_trigger", ObjectBody::named},
    Row{"eye_of_the_magi", ObjectBody::stationary},
    Row{"faerie_ring", ObjectBody::stationary},
    Row{"ferry", ObjectBody::named},
    Row{"flotsam", ObjectBody::stationary},
    Row{"fountain", ObjectBody::stationary},
    Row{"garrison", ObjectBody::ownable},
    Row{"gateway", ObjectBody::named},
    Row{"hut_of_the_magi", ObjectBody::stationary},
    Row{"idol_of_fortune", ObjectBody::stationary},
    Row{"keymaster_tent", ObjectBody::stationary},
    Row{"lighthouse", ObjectBody::weekly_generator},
    Row{"magic_gem", ObjectBody::stationary},
    Row{"mana_recharger", ObjectBody::stationary},
    Row{"material_pile", ObjectBody::stationary},
    Row{"medicine_wagon", ObjectBody::stationary},
    Row{"mercenary_camp", ObjectBody::stationary},
    Row{"mine", ObjectBody::ownable},
    Row{"movement_booster", ObjectBody::stationary},
    Row{"nothing", ObjectBody::stationary},
    Row{"obelisk", ObjectBody::stationary},
    Row{"obelisk_marker", ObjectBody::obelisk_marker},
    Row{"ocean_bottle", ObjectBody::sign},
    Row{"outpost", ObjectBody::outpost},
    Row{"oyster", ObjectBody::stationary},
    Row{"pandoras_box", ObjectBody::named},
    Row{"pathfinder", ObjectBody::stationary},
    Row{"port", ObjectBody::stationary},
    Row{"prison", ObjectBody::prison},
    Row{"quest_gate", ObjectBody::quest_gate},
    Row{"quest_guard", ObjectBody::quest_guard},
    Row{"rainbow", ObjectBody::stationary},
    Row{"random_artifact", ObjectBody::stationary},
    Row{"random_conservatory", ObjectBody::random_conservatory},
    Row{"random_creature_dwelling", ObjectBody::random_dwelling},
    Row{"random_material_pile", ObjectBody::stationary},
    Row{"random_monster", ObjectBody::random_monster},
    Row{"random_potion", ObjectBody::stationary},
    Row{"random_random_monster", ObjectBody::random_monster},
    Row{"random_shrine", ObjectBody::random_shrine},
    Row{"random_staff", ObjectBody::stationary},
    Row{"random_teacher", ObjectBody::random_altar},
    Row{"random_temple", ObjectBody::random_temple},
    Row{"random_tome", ObjectBody::stationary},
    Row{"random_town", ObjectBody::random_town},
    Row{"random_weekly_material_generator", ObjectBody::weekly_generator},
    Row{"sacred_grove", ObjectBody::stationary},
    Row{"sanctuary", ObjectBody::stationary},
    Row{"school", ObjectBody::skill_pair},
    Row{"seers_hut", ObjectBody::seers_hut},
    Row{"shipwreck_survivor", ObjectBody::stationary},
    Row{"shipyard", ObjectBody::weekly_generator},
    Row{"shrine", ObjectBody::shrine},
    Row{"sign", ObjectBody::sign},
    Row{"sphinx", ObjectBody::stationary},
    Row{"subterranean_gate", ObjectBody::named},
    Row{"system_object", ObjectBody::stationary},
    Row{"tavern", ObjectBody::stationary},
    Row{"teacher", ObjectBody::stationary},
    Row{"teleporter_entrance", ObjectBody::named},
    Row{"teleporter_exit", ObjectBody::named},
    Row{"temple", ObjectBody::stationary},
    Row{"tower", ObjectBody::stationary},
    Row{"town", ObjectBody::town},
    Row{"trading_post", ObjectBody::stationary},
    Row{"training_grounds", ObjectBody::stationary},
    Row{"treasure", ObjectBody::stationary},
    Row{"tree_of_knowledge", ObjectBody::stationary},
    Row{"vein", ObjectBody::stationary},
    Row{"victory_flag", ObjectBody::stationary},
    Row{"weekly_material_generator", ObjectBody::weekly_generator},
    Row{"whirlpool", ObjectBody::named},
    Row{"windmill", ObjectBody::weekly_generator},
};

constexpr std::array<std::string_view, kObjectBodies> kBodyNames{"stationary",
                                                                 "named",
                                                                 "sign",
                                                                 "obelisk_marker",
                                                                 "spell_artifact",
                                                                 "town",
                                                                 "random_town",
                                                                 "ownable",
                                                                 "abandoned_mine",
                                                                 "weekly_generator",
                                                                 "random_dwelling",
                                                                 "shrine",
                                                                 "random_shrine",
                                                                 "conservatory",
                                                                 "random_conservatory",
                                                                 "skill_pair",
                                                                 "random_altar",
                                                                 "random_temple",
                                                                 "random_monster",
                                                                 "quest_gate",
                                                                 "quest_guard",
                                                                 "seers_hut",
                                                                 "prison",
                                                                 "carryover_hero",
                                                                 "outpost",
                                                                 "army"};

}

std::string_view object_body_name(ObjectBody body) noexcept {
    const auto index = std::to_underlying(body);
    return index < kBodyNames.size() ? kBodyNames[index] : std::string_view{};
}

ObjectBody object_body_for_family(std::string_view family, std::string_view sub) noexcept {
    if (core::iequals(family, "artifact") && (core::iequals(sub, "parchment") || core::iequals(sub, "scroll")))
        return ObjectBody::spell_artifact;
    for (const Row& row : kFamilies)
        if (core::iequals(row.first, family))
            return row.second;
    return ObjectBody::stationary;
}

bool is_known_family(std::string_view family) noexcept {
    return std::ranges::any_of(kFamilies, [family](const Row& row) { return core::iequals(row.first, family); });
}

}
