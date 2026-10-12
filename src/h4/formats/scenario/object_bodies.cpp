#include "h4/formats/scenario/object_bodies.hpp"

#include <array>
#include <span>
#include <string_view>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/packed_bits.hpp"

namespace h4::formats {

namespace {

void town_chain(core::ByteReader& in, RecordReader& records, ScenarioObject& out, std::uint16_t v) {
    if (in.ok() && (v < 6 || v > 8)) {
        in.fail(core::ErrorCode::unsupported, "town version");
        return;
    }
    out.owner = body_owner(in);
    if (!in.ok())
        return;
    if (v >= 2)
        out.town_name = std::string{in.string16()};
    if (in.u8() != 0)
        records.army(out.army);
    if (v >= 3) {
        if (in.u8() != 0) {
            const auto mask = [&in] {
                std::uint64_t bits                     = 0;
                const std::span<const std::byte> bytes = in.bytes(6);
                for (std::uint8_t id = 0; in.ok() && id < kBuildingIds; ++id)
                    if (packed_bit(bytes, id))
                        bits |= std::uint64_t{1} << id;
                return bits;
            };
            out.town_buildings.built   = mask();
            out.town_buildings.enabled = mask();
            out.town_buildings.stored  = true;
        } else {
            out.town_buildings.built   = in.u8() != 0 ? std::uint64_t{1} << 3 : 0;
            out.town_buildings.enabled = (std::uint64_t{1} << kBuildingIds) - 1;
        }
    } else {
        out.town_buildings.built   = std::uint64_t{1} << 3;
        out.town_buildings.enabled = (std::uint64_t{1} << kBuildingIds) - 1;
    }
    records.attached_events(4);
    records.timed_events();
    records.triggerable_events();
    records.continuous_events();
    if (v >= 7 && in.u8() != 0) {
        records.disabled_skills();
        records.disabled_skills();
    }
    if (v >= 8)
        in.skip(1);
}

}

void read_object_body(core::ByteReader& in, RecordReader& records, ScenarioObject& out) {
    switch (out.body) {
    case ObjectBody::stationary: (void)body_version(in, 0); return;
    case ObjectBody::named:
    case ObjectBody::sign: {
        if (body_version(in, 1) != 0)
            out.message = std::string{in.string16()};
        return;
    }
    case ObjectBody::obelisk_marker:
        if (body_version(in, 1) == 1)
            (void)in.u16();
        return;
    case ObjectBody::spell_artifact:
        if (body_version(in, 1) != 0)
            (void)in.u32();
        return;
    case ObjectBody::town: {
        const std::uint16_t outer = in.u16();
        if (in.ok() && outer != 4) {
            in.fail(core::ErrorCode::unsupported, "town outer version");
            return;
        }
        records.spell_mask();
        records.spell_mask();
        town_chain(in, records, out, in.u16());
        return;
    }
    case ObjectBody::random_town: {
        const std::uint16_t v = body_version(in, 7);
        if (v < 7) {
            town_chain(in, records, out, v);
            return;
        }
        records.name_spec(out.name_spec);
        town_chain(in, records, out, in.u16());
        return;
    }
    case ObjectBody::ownable: {
        const std::uint16_t v = body_version(in, 2);
        if (v >= 1)
            out.owner = body_owner(in);
        if (v >= 2 && out.owner != kNoOwner)
            in.skip(1);
        if (v >= 2) {
            records.attached_events(4);
            records.continuous_events();
            records.timed_events();
            records.triggerable_events();
            if (in.u8() != 0)
                records.army(out.army);
        }
        return;
    }
    case ObjectBody::abandoned_mine:
        if (body_version(in, 1) == 1) {
            records.attached_events(4);
            records.continuous_events();
            records.timed_events();
            records.triggerable_events();
            if (in.u8() != 0)
                records.army(out.army);
        }
        return;
    case ObjectBody::weekly_generator: {
        const std::uint16_t v = body_version(in, 2);
        if (v >= 1)
            out.owner = body_owner(in);
        if (v >= 2)
            records.triggerable_events();
        return;
    }
    case ObjectBody::random_dwelling: {
        const std::uint16_t v = body_version(in, 3);
        if (v >= 1)
            out.owner = body_owner(in);
        if (v == 3)
            records.triggerable_events();
        if (v >= 2)
            records.name_spec(out.name_spec);
        return;
    }
    case ObjectBody::shrine: {
        const std::uint16_t v = body_version(in, 2);
        if (v == 1)
            in.skip(23);
        if (v == 2)
            records.spell_mask(&out.spells);
        return;
    }
    case ObjectBody::random_shrine: {
        const std::uint16_t v = body_version(in, 3);
        if (v == 1)
            in.skip(23);
        if (v >= 2)
            records.spell_mask();
        if (v == 3)
            records.name_spec(out.name_spec);
        return;
    }
    case ObjectBody::conservatory:
        if (body_version(in, 1) == 1) {
            records.spell_mask();
            records.spell_mask();
        }
        return;
    case ObjectBody::random_conservatory: {
        const std::uint16_t v = body_version(in, 2);
        if (v == 2) {
            records.spell_mask();
            records.spell_mask();
        }
        if (v >= 1)
            records.name_spec(out.name_spec);
        return;
    }
    case ObjectBody::skill_pair:
        if (body_version(in, 1) == 1) {
            records.disabled_skills();
            records.disabled_skills();
        }
        return;
    case ObjectBody::random_altar:
        if (body_version(in, 1) == 1) {
            out.scholar_mask = in.u16();
            records.name_spec(out.name_spec);
        }
        return;
    case ObjectBody::random_temple:
        if (body_version(in, 1) == 1)
            records.name_spec(out.name_spec);
        return;
    case ObjectBody::random_monster: {
        const std::uint16_t v = body_version(in, 4);
        if (v >= 3 && in.u8() != 0)
            in.skip(2);
        if (v >= 2) {
            records.attached_events(3);
            records.timed_events();
            records.triggerable_events();
            records.continuous_events();
        }
        if (v >= 1)
            records.name_spec(out.name_spec);
        if (v == 4) {
            out.band_low  = in.i32();
            out.band_high = in.i32();
            if (in.ok() && (out.band_low < 0 || out.band_high < out.band_low || (out.band_low > 0) != (out.band_high > 0)))
                in.fail(core::ErrorCode::corrupt, "monster strength band");
        }
        return;
    }
    case ObjectBody::quest_gate:
        if (body_version(in, 1) != 0)
            records.quest_core();
        return;
    case ObjectBody::quest_guard: {
        const std::uint16_t v = body_version(in, 2);
        if (v != 0) {
            records.quest_core();
            (void)in.string16();
        }
        if (v >= 2)
            records.script();
        return;
    }
    case ObjectBody::seers_hut: {
        const std::uint16_t v = body_version(in, 2);
        if (v == 0)
            return;
        records.quest_core();
        (void)in.string16();
        if (v == 1) {
            (void)in.string16();
            records.script();
            return;
        }
        records.script();
        (void)in.string16();
        return;
    }
    case ObjectBody::prison:
        if (body_version(in, 1) == 1) {
            out.army.slots[0].kind = ArmySlot::Kind::hero;
            records.hero(out.army.slots[0].hero);
        }
        return;
    case ObjectBody::carryover_hero: {
        const std::uint16_t v = body_version(in, 2);
        if (v >= 1) {
            out.owner = body_owner(in);
            (void)in.string16();
        }
        if (v >= 2) {
            if (in.u8() != 0)
                in.skip(2);
            records.attached_events(3);
            records.timed_events();
            records.triggerable_events();
            records.continuous_events();
        }
        return;
    }
    case ObjectBody::outpost:
        if (body_version(in, 1) >= 1)
            out.owner = body_owner(in);
        return;
    case ObjectBody::army: read_army_body(in, records, out); return;
    }
}

}
