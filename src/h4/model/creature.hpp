#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "h4/core/strong.hpp"
#include "h4/model/alignment.hpp"
#include "h4/model/resources.hpp"

namespace h4::model {

using CreatureKindId = core::Strong<struct CreatureKindIdTag, std::uint8_t>;
[[nodiscard]] constexpr std::optional<CreatureKindId> creature_kind_of(std::int32_t raw) noexcept {
    if (raw < 0 || raw > 0xFF)
        return std::nullopt;
    return CreatureKindId{static_cast<std::uint8_t>(raw)};
}

using CreatureLevel = core::Strong<struct CreatureLevelTag, std::uint8_t>;
using HitPoints     = core::Strong<struct HitPointsTag, std::int32_t, core::Additive>;
using Experience    = core::Strong<struct ExperienceTag, std::int32_t, core::Additive, core::Scalable>;

struct DamageRange {
    std::int32_t low                                                    = 0;
    std::int32_t high                                                   = 0;
    friend constexpr bool operator==(DamageRange, DamageRange) noexcept = default;
};

struct CreatureStats {
    CreatureLevel level;
    HitPoints hit_points;
    DamageRange damage;
    std::int32_t attack        = 0;
    std::int32_t defense       = 0;
    std::int32_t movement      = 0;
    std::int32_t speed         = 0;
    std::int32_t shots         = 0;
    std::int32_t spell_points  = 0;
    std::int32_t weekly_growth = 0;
    Experience experience;
};

struct CreatureNames {
    std::string singular;
    std::string plural;
    std::string long_help;
};

class CreatureType {
public:
    CreatureType(CreatureKindId id, std::string keyword, CreatureNames names, Alignment alignment, CreatureStats stats, ResourceBundle cost)
    : id_{id}, keyword_{std::move(keyword)}, names_{std::move(names)}, alignment_{alignment}, stats_{stats}, cost_{cost} {
    }

    [[nodiscard]] CreatureKindId id() const noexcept {
        return id_;
    }
    [[nodiscard]] std::string_view keyword() const noexcept {
        return keyword_;
    }
    [[nodiscard]] std::string_view name() const noexcept {
        return names_.singular;
    }
    [[nodiscard]] std::string_view plural_name() const noexcept {
        return names_.plural;
    }
    [[nodiscard]] std::string_view name_for(std::int64_t count) const noexcept {
        return count == 1 ? name() : plural_name();
    }
    [[nodiscard]] std::string_view long_help() const noexcept {
        return names_.long_help;
    }
    [[nodiscard]] Alignment alignment() const noexcept {
        return alignment_;
    }
    [[nodiscard]] const CreatureStats& stats() const noexcept {
        return stats_;
    }
    [[nodiscard]] const ResourceBundle& cost() const noexcept {
        return cost_;
    }

private:
    CreatureKindId id_;
    std::string keyword_;
    CreatureNames names_;
    Alignment alignment_;
    CreatureStats stats_;
    ResourceBundle cost_;
};

}
