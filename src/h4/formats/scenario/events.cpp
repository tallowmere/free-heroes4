#include "h4/formats/scenario/events.hpp"

#include "h4/core/byte_reader.hpp"
#include "h4/formats/scenario/records.hpp"
#include "h4/formats/scenario/script.hpp"

namespace h4::formats {

namespace {

constexpr std::size_t kMinEventBytes = 2 + 4 + 2 + 28;

void read_base(core::ByteReader& in, ScenarioEvent& event) {
    read_script(in, event.script_messages, 0, &event.removes);
    event.message = in.string16();
    for (std::int32_t& amount : event.gift)
        amount = static_cast<std::int32_t>(in.u32());
}

void read_audience(core::ByteReader& in, ScenarioEvent& event) {
    event.audience.players   = in.u8();
    const std::uint8_t flags = in.u8();
    event.audience.humans    = (flags & 1U) != 0;
    event.audience.computers = (flags & 2U) != 0;
}

}

bool EventAudience::admits(std::uint8_t colour, bool human) const noexcept {
    if (colour >= 8)
        return false;
    if (!(human ? humans : computers))
        return false;
    return ((players >> colour) & 1U) != 0;
}

std::vector<std::string> ScenarioEvent::words() const {
    std::vector<std::string> out;
    if (!message.empty())
        out.push_back(message);
    out.insert(out.end(), script_messages.begin(), script_messages.end());
    return out;
}

core::Result<ScenarioEvents> ScenarioEvents::parse(std::span<const std::byte> payload, std::size_t offset) {
    core::ByteReader in{payload, "scenario events", offset};
    ScenarioEvents out;

    for (std::size_t n = in.bounded_count(in.u16(), kMinEventBytes); in.ok() && n-- > 0;) {
        ScenarioEvent event;
        read_marker(in, 0, "event marker");
        read_base(in, event);
        event.name   = in.string16();
        event.day    = in.u16();
        event.repeat = in.u16();
        read_audience(in, event);
        out.timed_.push_back(std::move(event));
    }
    for (std::size_t n = in.bounded_count(in.u16(), kMinEventBytes); in.ok() && n-- > 0;) {
        ScenarioEvent event;
        read_marker(in, 0, "event marker");
        read_base(in, event);
        event.name = in.string16();
        out.triggerable_.push_back(std::move(event));
    }
    for (std::size_t n = in.bounded_count(in.u16(), 6); in.ok() && n-- > 0;) {
        ScenarioEvent event;
        read_marker(in, 0, "event marker");
        read_script(in, event.script_messages);
        event.name = in.string16();
        out.continuous_.push_back(std::move(event));
    }
    for (std::size_t n = in.bounded_count(in.u16(), kMinEventBytes); in.ok() && n-- > 0;) {
        ScenarioEvent event;
        const std::uint16_t kind = read_marker(in, 1, "placed event marker");
        if (!in.ok())
            break;
        read_base(in, event);
        if (kind == 1)
            read_audience(in, event);
        else
            event.audience = EventAudience{0x3F, true, true};
        event.name = in.string16();
        out.placed_.push_back(std::move(event));
    }

    out.end_ = in.position();
    return in.finish(std::move(out), false);
}

}
