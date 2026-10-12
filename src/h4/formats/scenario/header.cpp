#include "h4/formats/scenario/header.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>

#include "h4/core/byte_reader.hpp"
#include "h4/formats/payload_version.hpp"

namespace h4::formats {

namespace {

constexpr std::array<std::uint8_t, 10> kMagic{'H', '4', 'C', 'A', 'M', 'P', 'A', 'I', 'G', 'N'};
constexpr std::size_t kContainerBytes  = 15;
constexpr std::uint16_t kOldestVersion = 20;
constexpr std::uint16_t kNewestVersion = 29;
constexpr std::uint16_t kMaxGridDimension = 4096;
constexpr std::uint8_t kMaxPlayers        = 6;
constexpr std::uint8_t kColourCount       = 6;
constexpr std::uint8_t kDifficulties      = 5;

std::optional<std::string> optional_text(core::ByteReader& in) {
    if (in.u8() == 0)
        return std::nullopt;
    return std::string{in.string16()};
}

void skip_name_triple(core::ByteReader& in) {
    if (in.u8() == 0)
        return;
    if (in.u16() != 0)
        in.fail(core::ErrorCode::corrupt, "name triple marker is not zero");
    for (int i = 0; i < 3; ++i)
        (void)in.string16();
}

std::vector<ScenarioPlayerSlot> read_players(core::ByteReader& in) {
    const std::uint8_t count = in.u8();
    if (in.ok() && (count == 0 || count > kMaxPlayers))
        in.fail(core::ErrorCode::corrupt, std::format("{} players", count));
    std::vector<ScenarioPlayerSlot> players;
    std::array<bool, kColourCount> taken{};
    for (std::uint8_t i = 0; in.ok() && i < count; ++i) {
        ScenarioPlayerSlot slot;
        slot.colour     = in.u8();
        slot.any_player = in.u8() != 0;
        (void)in.u8();
        if (in.u8() == 0)
            slot.alignments = in.u8();
        if (slot.colour >= kColourCount || taken[slot.colour]) {
            in.fail(core::ErrorCode::corrupt, std::format("player colour {}", slot.colour));
            break;
        }
        taken[slot.colour] = true;
        players.push_back(slot);
    }
    if (in.ok() && std::ranges::none_of(players, &ScenarioPlayerSlot::any_player))
        in.fail(core::ErrorCode::corrupt, "no player slot is open to a human");
    return players;
}

}

core::Result<std::span<const std::byte>> ScenarioHeader::embedded_file(std::span<const std::byte> resource) {
    constexpr std::size_t kResourceHeader = 16;
    core::ByteReader in{resource, "game map resource", kResourceHeader - 4};
    const std::uint32_t length = in.u32();
    if (!in.ok() || length != resource.size() - kResourceHeader)
        return core::fail(core::ErrorCode::corrupt, "game map resource length");
    return resource.subspan(kResourceHeader);
}

core::Result<std::span<const std::byte>> ScenarioHeader::container_stream(std::span<const std::byte> file) {
    if (file.size() <= kContainerBytes || !core::starts_with_magic(file, kMagic))
        return core::fail(core::ErrorCode::corrupt, "not an H4CAMPAIGN container");
    return file.subspan(kContainerBytes);
}

core::Result<ScenarioHeader> ScenarioHeader::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, "scenario header"};
    ScenarioHeader h;
    h.version_ = read_version(in, kOldestVersion, kNewestVersion, "scenario header version");
    if (h.version_ >= 28)
        h.content_tag_ = in.u16();
    h.grid_dimension_ = in.u16();
    if (in.ok() && h.grid_dimension_ > kMaxGridDimension)
        in.fail(core::ErrorCode::corrupt, std::format("grid dimension {}", h.grid_dimension_));
    h.levels_       = in.u8();
    h.terrain_seed_ = in.u32();
    h.players_      = read_players(in);
    h.name_         = in.string16();

    const std::uint8_t difficulty = in.u8();
    if (in.ok() && difficulty >= kDifficulties)
        in.fail(core::ErrorCode::corrupt, std::format("difficulty {}", difficulty));
    h.difficulty_  = static_cast<ScenarioDifficulty>(difficulty);
    h.description_ = in.string16();

    if (in.u16() != 0 && in.ok())
        in.fail(core::ErrorCode::corrupt, "team marker is not zero");
    const std::size_t teams = in.bounded_count(in.u8(), 1);
    for (std::size_t i = 0; i < teams; ++i)
        h.teams_.push_back(in.u8());

    h.loss_    = optional_text(in);
    h.victory_ = optional_text(in);
    if (h.version_ >= 25)
        (void)in.u8();
    if (h.version_ >= 24) {
        skip_name_triple(in);
        skip_name_triple(in);
    }
    if (h.version_ >= 27)
        (void)in.string16();
    if (h.version_ >= 29)
        h.multiplayer_ = in.u8() != 0;
    h.body_offset_ = in.position();
    return in.finish(std::move(h), false);
}

std::size_t ScenarioHeader::human_slots() const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(players_, &ScenarioPlayerSlot::any_player));
}

std::size_t ScenarioHeader::alliance_count() const noexcept {
    unsigned occupied = 0;
    for (const ScenarioPlayerSlot& slot : players_)
        occupied |= 1U << slot.colour;
    return static_cast<std::size_t>(std::ranges::count_if(teams_, [occupied](std::uint8_t mask) {
        return (mask & occupied) != 0;
    }));
}

}
