#include "h4/formats/adv_actor.hpp"

#include <array>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/payload_version.hpp"

namespace h4::formats {

namespace {

constexpr std::array<std::string_view, kActorGroups>
    kGroupNames{"wait", "wait_and_base", "fidget", "prewalk", "walk", "postwalk", "full_walk", "base_frame", "attack"};

[[nodiscard]] int group_index(std::string_view name) noexcept {
    if (core::iequals(name, "stand"))
        return 0;
    const auto i = core::index_of(kGroupNames, name);
    return i.has_value() ? static_cast<int>(*i) : -1;
}

}

std::string_view actor_group_name(ActorGroup group) noexcept {
    const std::size_t i = std::to_underlying(group);
    return i < kGroupNames.size() ? kGroupNames[i] : std::string_view{};
}

std::string_view AdvActor::sequence(ActorGroup group, ActorDirection direction) const noexcept {
    const std::size_t g = std::to_underlying(group);
    const std::size_t d = std::to_underlying(direction);
    if (g >= groups_.size() || d >= kActorDirections)
        return {};
    return groups_[g].cells[d];
}

std::uint8_t AdvActor::frame_rate(ActorGroup group) const noexcept {
    const std::size_t g = std::to_underlying(group);
    return g < groups_.size() ? groups_[g].rate : 0;
}

core::Result<AdvActor> AdvActor::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, "adv_actor"};
    AdvActor out;

    out.version_ = read_version(in, 0, 3, "adv_actor version");
    if (!in.ok())
        return in.finish(std::move(out), false);

    out.depth_ = in.u8();
    if (out.version_ >= 2)
        out.walk_.prewalk = in.u16();
    out.walk_.walk     = in.u16();
    out.walk_.postwalk = in.u16();

    for (std::size_t n = in.bounded_count(in.u16(), 3); in.ok() && n-- > 0;) {
        const std::string_view name = in.string16();
        const int index             = group_index(name);
        const std::uint8_t rate     = in.u8();
        for (std::size_t d = 0; in.ok() && d < kActorDirections; ++d) {
            const std::string_view cell = in.string16();
            if (index < 0)
                continue;
            out.groups_[static_cast<std::size_t>(index)].cells[d] = std::string{cell};
        }
        if (index < 0)
            continue;
        Group& group  = out.groups_[static_cast<std::size_t>(index)];
        group.present = true;
        group.rate    = rate;
    }

    if (out.version_ >= 1)
        out.anchor_ = {in.i32(), in.i32()};
    if (out.version_ >= 3)
        out.flag_point_ = {in.i32(), in.i32()};
    return in.finish(std::move(out), true);
}

}
