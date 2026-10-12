#include "h4/formats/combat_actor.hpp"

#include <array>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/payload_version.hpp"

namespace h4::formats {

namespace {

constexpr std::array<std::string_view, kCombatStates> kStateNames{"block",
                                                                  "cast_spell",
                                                                  "die",
                                                                  "fidget",
                                                                  "flinch",
                                                                  "melee",
                                                                  "melee_down",
                                                                  "melee_up",
                                                                  "postwalk",
                                                                  "prewalk",
                                                                  "walk",
                                                                  "full_walk",
                                                                  "ranged",
                                                                  "base_frame",
                                                                  "wait",
                                                                  "wait_and_base"};

[[nodiscard]] int state_index(std::string_view name) noexcept {
    const auto i = core::index_of(kStateNames, name);
    return i.has_value() ? static_cast<int>(*i) : -1;
}

}

std::string_view CombatActor::sequence(CombatState state, std::size_t facing) const noexcept {
    const auto s = std::to_underlying(state);
    if (s >= states_.size() || facing >= kCombatFacings)
        return {};
    return states_[s].facings[facing];
}

std::uint8_t CombatActor::rate(CombatState state) const noexcept {
    const auto s = std::to_underlying(state);
    return s < states_.size() ? states_[s].rate : 0;
}

core::Result<CombatActor> CombatActor::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, "combat_actor"};
    CombatActor out;
    out.version_ = read_version(in, 0, 6, "combat_actor version");
    if (!in.ok())
        return in.finish(std::move(out), false);
    if (out.version_ >= 1)
        out.height_ = in.u8();
    if (out.version_ >= 2)
        (void)in.u8();
    if (out.version_ >= 3)
        (void)in.bytes(6);
    if (out.version_ >= 6)
        (void)in.bytes(96);
    const std::size_t records = in.bounded_count(in.u16(), 3);
    for (std::size_t r = 0; in.ok() && r < records; ++r) {
        const int state         = state_index(in.string16());
        const std::uint8_t rate = in.u8();
        if (out.version_ > 4)
            (void)in.u8();
        std::array<std::string, kCombatFacings> facings{};
        for (std::string& facing : facings)
            facing = std::string{in.string16()};
        if (state < 0)
            continue;
        State& slot  = out.states_[static_cast<std::size_t>(state)];
        slot.rate    = rate;
        slot.facings = std::move(facings);
    }
    if (out.version_ >= 4) {
        out.anchor_.x = in.i32();
        out.anchor_.y = in.i32();
    }
    return in.finish(std::move(out));
}

}
