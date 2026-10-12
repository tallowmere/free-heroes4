#include "h4/model/scenario.hpp"

#include <format>
#include <span>
#include <utility>

#include "h4/formats/scenario/header.hpp"
#include "h4/io/compression.hpp"
#include "h4/io/mapped_file.hpp"

namespace h4::model {

namespace {

constexpr std::size_t kHeaderGuess = std::size_t{1} << 16;
constexpr std::size_t kLimit       = std::size_t{1} << 25;

core::Result<std::vector<std::byte>> inflate_file(std::span<const std::byte> file, std::size_t limit) {
    auto stream = formats::ScenarioHeader::container_stream(file);
    if (!stream)
        return std::unexpected{stream.error()};
    return io::inflate_prefix(*stream, limit);
}

ScenarioSummary summarise(const std::filesystem::path& file, const formats::ScenarioHeader& h) {
    ScenarioSummary s;
    s.path        = file;
    s.name        = h.name();
    s.description = h.description();
    s.size        = map_size_of(h.grid_dimension());
    s.difficulty  = static_cast<MapDifficulty>(h.difficulty());
    s.players     = h.players().size();
    s.humans      = h.human_slots();
    s.alliances   = h.alliance_count();
    if (h.content_tag() <= std::to_underlying(MapEdition::winds_of_war))
        s.edition = static_cast<MapEdition>(h.content_tag());
    s.multiplayer = h.multiplayer();
    for (const formats::ScenarioPlayerSlot& player : h.players()) {
        ScenarioSlot slot;
        slot.colour        = static_cast<PlayerColour>(player.colour);
        slot.human_capable = player.any_player;
        slot.alignments    = player.alignments;
        const auto masks   = h.team_masks();
        for (std::size_t t = 0; t < masks.size(); ++t)
            if ((masks[t] >> player.colour) & 1U) {
                slot.team = t;
                break;
            }
        s.slots.push_back(slot);
    }
    s.loss_text    = h.loss_text();
    s.victory_text = h.victory_text();
    return s;
}

}

MapSize map_size_of(std::uint16_t grid_dimension) noexcept {
    if (grid_dimension < 77)
        return MapSize::small;
    if (grid_dimension < 153)
        return MapSize::medium;
    if (grid_dimension < 229)
        return MapSize::large;
    return MapSize::extra_large;
}

bool ScenarioMessage::admits(PlayerColour colour, bool human) const noexcept {
    return audience.admits(std::to_underlying(colour), human);
}

core::Result<ScenarioSummary> read_scenario_summary(const std::filesystem::path& file) {
    auto mapped = io::MappedFile::open(file);
    if (!mapped)
        return std::unexpected{mapped.error()};
    return read_scenario_summary(mapped->bytes(), file);
}

core::Result<ScenarioSummary> read_scenario_summary(std::span<const std::byte> file, const std::filesystem::path& path) {
    for (const std::size_t limit : {kHeaderGuess, kLimit}) {
        auto payload = inflate_file(file, limit);
        if (!payload)
            return std::unexpected{payload.error()};
        auto header = formats::ScenarioHeader::parse(*payload);
        if (header)
            return summarise(path, *header);
        if (payload->size() < limit)
            return std::unexpected{header.error()};
    }
    return core::fail(core::ErrorCode::corrupt, std::format("{}: header too long", path.string()));
}

core::Result<ScenarioSummary> read_tutorial_summary(std::span<const std::byte> resource) {
    auto file = formats::ScenarioHeader::embedded_file(resource);
    if (!file)
        return std::unexpected{file.error()};
    return read_scenario_summary(*file, kTutorialPath);
}

core::Result<Scenario> load_tutorial(std::span<const std::byte> resource) {
    auto file = formats::ScenarioHeader::embedded_file(resource);
    if (!file)
        return std::unexpected{file.error()};
    return Scenario::parse(*file, kTutorialPath);
}

core::Result<Scenario> Scenario::load(const std::filesystem::path& file) {
    auto mapped = io::MappedFile::open(file);
    if (!mapped)
        return std::unexpected{mapped.error()};
    return parse(mapped->bytes(), file);
}

core::Result<Scenario> Scenario::parse(std::span<const std::byte> file, const std::filesystem::path& path) {
    auto payload = inflate_file(file, kLimit);
    if (!payload)
        return std::unexpected{payload.error()};
    auto header = formats::ScenarioHeader::parse(*payload);
    if (!header)
        return std::unexpected{header.error()};
    auto events = formats::ScenarioEvents::parse(*payload, header->body_offset());
    if (!events)
        return std::unexpected{events.error()};
    auto map = Map::parse(*payload, *header, events->end());
    if (!map)
        return std::unexpected{map.error()};

    Scenario scenario;
    scenario.summary_ = summarise(path, *header);
    scenario.grid_    = header->grid_dimension();
    scenario.levels_  = header->levels();
    scenario.map_     = std::move(*map);
    for (const formats::ScenarioEvent& event : events->placed()) {
        PlacedEvent placed{event.name, event.words(), {}, event.audience, event.removes};
        for (std::size_t r = 0; r < kResourceCount; ++r)
            placed.gift = placed.gift.with(static_cast<Resource>(r), ResourceAmount{event.gift[r]});
        scenario.placed_.push_back(std::move(placed));
    }
    for (const formats::ScenarioEvent& event : events->timed())
        for (std::string& words : event.words())
            scenario.timed_.push_back(ScenarioMessage{std::move(words), event.day, event.repeat, event.audience});
    return scenario;
}

bool PlacedEvent::admits(PlayerColour colour, bool human) const noexcept {
    return audience.admits(std::to_underlying(colour), human);
}

bool ScenarioMessage::fires_on(std::int32_t today) const noexcept {
    if (today == day)
        return true;
    return today > day && repeat != 0 && (today - day) % repeat == 0;
}

std::vector<std::string> Scenario::messages_on(std::int32_t day, PlayerColour colour, bool human) const {
    std::vector<std::string> out;
    for (const ScenarioMessage& message : timed_)
        if (message.fires_on(day) && message.admits(colour, human))
            out.push_back(message.text);
    return out;
}

}
