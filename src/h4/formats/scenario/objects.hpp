#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/formats/scenario/families.hpp"
#include "h4/formats/scenario/header.hpp"
#include "h4/formats/scenario/records.hpp"

namespace h4::formats {

inline constexpr std::uint8_t kNoOwner     = 6;
inline constexpr std::uint8_t kBuildingIds = 43;

struct ArmyPatrol {
    std::int32_t kind  = 0;
    std::int8_t radius = -1;
};

struct TownBuildings {
    std::uint64_t built   = 0;
    std::uint64_t enabled = 0;
    bool stored = false;

    [[nodiscard]] bool has(std::uint8_t id) const noexcept {
        return id < kBuildingIds && (built >> id & 1U) != 0;
    }
    [[nodiscard]] bool allows(std::uint8_t id) const noexcept {
        return id < kBuildingIds && (enabled >> id & 1U) != 0;
    }
};

struct ScenarioObject {
    std::int32_t x     = 0;
    std::int32_t y     = 0;
    std::int32_t level = 0;

    std::string family;
    std::string sub;
    std::string variant;
    std::string model;

    ObjectBody body    = ObjectBody::army;
    std::uint8_t owner = kNoOwner;

    bool is_army = false;
    ArmyRecord army;
    std::int8_t alignment = -1;
    bool starting_troops = false;
    ArmyPatrol patrol;

    SpellMask spells;
    std::string town_name;
    TownBuildings town_buildings;

    std::string message;
    NameSpec name_spec;

    std::int32_t subtype = 0;

    std::int32_t band_low = 0, band_high = 0;

    std::uint16_t scholar_mask = 0;

    [[nodiscard]] bool is_town() const noexcept {
        return body == ObjectBody::town || body == ObjectBody::random_town;
    }
    [[nodiscard]] const HeroRecord* hero() const noexcept {
        return army.leader();
    }
};

class ScenarioObjects {
public:
    [[nodiscard]] static core::Result<ScenarioObjects>
    parse(std::span<const std::byte> payload, std::size_t offset, const ScenarioHeader& header);

    [[nodiscard]] std::span<const ScenarioObject> objects() const noexcept {
        return objects_;
    }
    [[nodiscard]] std::size_t end() const noexcept {
        return end_;
    }

private:
    ScenarioObjects() = default;

    std::vector<ScenarioObject> objects_;
    std::size_t end_ = 0;
};

}
