#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::formats {

enum class ActorGroup : std::uint8_t {
    wait,
    wait_and_base,
    fidget,
    prewalk,
    walk,
    postwalk,
    full_walk,
    base_frame,
    attack
};

enum class ActorDirection : std::uint8_t { ne, e, se, s, sw, w, nw, n };

inline constexpr std::size_t kActorGroups     = 9;
inline constexpr std::size_t kActorDirections = 8;

[[nodiscard]] std::string_view actor_group_name(ActorGroup group) noexcept;

class AdvActor {
public:
    [[nodiscard]] static core::Result<AdvActor> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::uint16_t version() const noexcept {
        return version_;
    }
    [[nodiscard]] core::Point anchor() const noexcept {
        return anchor_;
    }
    [[nodiscard]] core::Point flag_point() const noexcept {
        return flag_point_;
    }

    [[nodiscard]] std::string_view sequence(ActorGroup group, ActorDirection direction) const noexcept;
    [[nodiscard]] std::uint8_t frame_rate(ActorGroup group) const noexcept;

    struct WalkGeometry {
        std::int32_t prewalk  = 0x10;
        std::int32_t walk     = 0x20;
        std::int32_t postwalk = 0x10;
    };
    [[nodiscard]] const WalkGeometry& walk_geometry() const noexcept {
        return walk_;
    }
    [[nodiscard]] std::int32_t depth() const noexcept {
        return depth_;
    }

private:
    AdvActor() = default;

    struct Group {
        bool present      = false;
        std::uint8_t rate = 0;
        std::array<std::string, kActorDirections> cells;
    };

    std::uint16_t version_ = 0;
    core::Point anchor_;
    core::Point flag_point_;
    WalkGeometry walk_;
    std::int32_t depth_ = 1;
    std::array<Group, kActorGroups> groups_;
};

}
