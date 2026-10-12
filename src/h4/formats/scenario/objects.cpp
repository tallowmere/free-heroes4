#include "h4/formats/scenario/objects.hpp"

#include <format>
#include <string_view>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/scenario/object_bodies.hpp"
#include "h4/formats/scenario/records.hpp"

namespace h4::formats {

namespace {

constexpr std::size_t kMinObjectBytes = 13;
constexpr std::size_t kOracleColours  = 12;

void bitset(core::ByteReader& in, std::size_t max_bits) {
    const std::uint16_t bits = in.u16();
    if (in.ok() && bits > max_bits) {
        in.fail(core::ErrorCode::corrupt, "bitset too wide");
        return;
    }
    in.skip((bits + 7U) / 8U);
}

void reward(core::ByteReader& in, RecordReader& records) {
    read_marker(in, 0, "section marker");
    const std::string_view kind = in.string16();
    if (!in.ok())
        return;
    if (core::iequals(kind, "artifact")) {
        (void)records.artifact_instance();
        (void)in.u32();
        return;
    }
    if (core::iequals(kind, "material")) {
        in.skip(5);
        return;
    }
    in.fail(core::ErrorCode::corrupt, "unknown oracle reward");
}

void read_sections(core::ByteReader& in, RecordReader& records, const ScenarioHeader& header) {
    const std::uint16_t v = header.version();

    for (std::size_t i = 0; in.ok() && i < kOracleColours; ++i) {
        read_marker(in, 0, "section marker");
        const std::uint8_t gate = in.u8();
        (void)in.u16();
        if (gate == 0)
            continue;
        for (std::size_t n = in.bounded_count(in.u16(), 4); in.ok() && n-- > 0;)
            reward(in, records);
    }

    if (v >= 16)
        for (std::size_t i = 0; in.ok() && i < header.players().size(); ++i) {
            const std::uint8_t colour = in.u8();
            if (in.ok() && colour >= 6) {
                in.fail(core::ErrorCode::corrupt, "hero preference colour");
                return;
            }
            if (in.u8() != 0)
                (void)in.u16();
            if (v >= 23)
                in.skip(1);
        }

    if (v >= 18) {
        if (in.u8() != 0) {
            (void)in.u16();
            bitset(in, 248);
        }
        if (in.u8() != 0) {
            const std::uint16_t n = in.u16();
            in.skip(n);
        }
        if (in.u8() != 0)
            records.disabled_skills();
        if (in.u8() != 0)
            bitset(in, 188);
    }

    if (v >= 21 && v < 24)
        for (int i = 0; in.ok() && i < 2; ++i)
            if (in.u8() != 0)
                for (int j = 0; in.ok() && j < 3; ++j)
                    (void)in.string16();

    if (v >= 22)
        for (int i = 0; in.ok() && i < 2; ++i) {
            (void)in.u16();
            bitset(in, 248);
        }

    if (v >= 23) {
        for (int list = 0; in.ok() && list < 3; ++list) {
            read_marker(in, 0, "section marker");
            const std::size_t limit = list == 0 ? 248 : 188;
            for (std::size_t n = in.bounded_count(in.u16(), 3); in.ok() && n-- > 0;) {
                const std::uint16_t id  = in.u16();
                const std::uint8_t flag = in.u8();
                if (in.ok() && (id >= limit || flag < 1 || flag > 2))
                    in.fail(core::ErrorCode::corrupt, "hero or spell flag map");
            }
        }
    }
}

void read_classification(core::ByteReader& in, ScenarioObject& out) {
    read_marker(in, 0, "section marker");
    (void)in.u16();
    out.family = std::string{in.string16()};
    (void)in.u16();
    out.sub = std::string{in.string16()};
    out.subtype = in.u16();
    out.variant = std::string{in.string16()};
}

}

core::Result<ScenarioObjects>
ScenarioObjects::parse(std::span<const std::byte> payload, std::size_t offset, const ScenarioHeader& header) {
    core::ByteReader in{payload, "scenario objects", offset};
    ScenarioObjects out;
    std::vector<std::string> words;
    RecordReader records{in, &words};

    read_sections(in, records, header);

    const std::size_t count = in.bounded_count(in.u32(), kMinObjectBytes);
    out.objects_.reserve(count);
    for (std::size_t i = 0; in.ok() && i < count; ++i) {
        ScenarioObject object;
        object.x       = in.i32();
        object.y       = in.i32();
        object.level   = in.i32();
        object.is_army = in.u8() != 0;
        if (object.is_army) {
            object.body = ObjectBody::army;
        } else {
            object.model = std::string{in.string16()};
            if (header.version() >= 14)
                read_classification(in, object);
            if (in.ok() && !is_known_family(object.family)) {
                in.fail(core::ErrorCode::unsupported, std::format("object family '{}'", object.family));
                break;
            }
            object.body = object_body_for_family(object.family, object.sub);
        }
        read_object_body(in, records, object);
        if (!in.ok()) {
            const core::Error error = *in.error();
            return core::fail(error.code(),
                              std::format("{} (object {} of {}, '{}' {}/{}, body {})",
                                          error.message(),
                                          i,
                                          count,
                                          object.model,
                                          object.family,
                                          object.sub,
                                          object_body_name(object.body)));
        }
        out.objects_.push_back(std::move(object));
    }
    out.end_ = in.position();
    return in.finish(std::move(out), false);
}

}
