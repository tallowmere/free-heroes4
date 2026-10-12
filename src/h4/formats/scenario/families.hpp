#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace h4::formats {

enum class ObjectBody : std::uint8_t {
    stationary,
    named,
    sign,
    obelisk_marker,
    spell_artifact,
    town,
    random_town,
    ownable,
    abandoned_mine,
    weekly_generator,
    random_dwelling,
    shrine,
    random_shrine,
    conservatory,
    random_conservatory,
    skill_pair,
    random_altar,
    random_temple,
    random_monster,
    quest_gate,
    quest_guard,
    seers_hut,
    prison,
    carryover_hero,
    outpost,
    army
};
inline constexpr std::size_t kObjectBodies = std::to_underlying(ObjectBody::army) + 1;

[[nodiscard]] std::string_view object_body_name(ObjectBody body) noexcept;
[[nodiscard]] ObjectBody object_body_for_family(std::string_view family, std::string_view sub) noexcept;

[[nodiscard]] bool is_known_family(std::string_view family) noexcept;

}
