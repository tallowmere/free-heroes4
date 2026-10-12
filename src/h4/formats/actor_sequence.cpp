#include "h4/formats/actor_sequence.hpp"

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

core::Result<ActorSequence> ActorSequence::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, "actor_sequence"};
    ActorSequence out;

    const std::uint16_t container = in.u16();
    if (in.ok() && container != 2)
        in.fail(core::ErrorCode::unsupported, "actor_sequence container version");
    const std::uint16_t base = in.u16();
    if (in.ok() && base > 3)
        in.fail(core::ErrorCode::unsupported, "actor_sequence base version");
    if (!in.ok())
        return in.finish(std::move(out), false);
    if (base > 2)
        in.skip(6);

    auto body = read_sub_image_body(payload, in, base == 1);
    if (!body)
        return std::unexpected{body.error()};
    out.group_  = std::move(body->group);
    out.anchor_ = body->anchor;

    for (std::size_t n = in.bounded_count(in.u16(), 4); in.ok() && n-- > 0;) {
        (void)in.string16();
        (void)in.u16();
    }

    out.layers_ = classify_sub_image(out.group_);
    return in.finish(std::move(out), true);
}

}
