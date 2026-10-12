#include "h4/core/byte_reader.hpp"
#include "h4/formats/payload_version.hpp"
#include "h4/formats/scenario/object_bodies.hpp"

namespace h4::formats {

std::uint16_t body_version(core::ByteReader& in, std::uint16_t max) {
    return read_version(in, 0, max, "object body version");
}

std::uint8_t body_owner(core::ByteReader& in) {
    const std::uint8_t owner = in.u8();
    if (in.ok() && owner > kNoOwner)
        in.fail(core::ErrorCode::corrupt, "object owner");
    return owner;
}

void read_army_body(core::ByteReader& in, RecordReader& records, ScenarioObject& out) {
    const std::uint16_t v = body_version(in, 9);
    if (v == 0)
        return;
    if (v >= 2)
        out.owner = body_owner(in);
    if (v < 3 && in.u8() != 0) {
        in.fail(core::ErrorCode::unsupported, "legacy army record");
        return;
    }
    records.army(out.army);
    const bool authored = v == 4 || (v >= 5 && in.u8() != 0);
    if (authored) {
        out.patrol.kind   = in.i8() + 1;
        out.patrol.radius = in.i8();
        if (out.patrol.radius < 0)
            out.patrol.kind = 0;
        if (out.patrol.kind == 1)
            out.patrol.radius = 3;
    } else if (v >= 5 && out.owner == kNoOwner)
        out.patrol = ArmyPatrol{1, 3};
    if (v >= 6) {
        records.attached_events(3);
        records.timed_events();
        records.triggerable_events();
        records.continuous_events();
    }
    if (v >= 7)
        out.alignment = in.i8();
    if (v >= 8)
        in.skip(1);
    if (v >= 9)
        out.starting_troops = in.u8() != 0;
}

}
