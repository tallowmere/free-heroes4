#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {

enum class ScenarioDifficulty : std::uint8_t { easy, normal, hard, expert, impossible };

struct ScenarioPlayerSlot {
    std::uint8_t colour     = 0;
    bool any_player         = false;
    std::uint8_t alignments = 0;
};

class ScenarioHeader {
public:
    [[nodiscard]] static core::Result<std::span<const std::byte>> embedded_file(std::span<const std::byte> resource);

    [[nodiscard]] static core::Result<std::span<const std::byte>> container_stream(std::span<const std::byte> file);

    [[nodiscard]] static core::Result<ScenarioHeader> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::uint16_t version() const noexcept {
        return version_;
    }
    [[nodiscard]] std::uint16_t content_tag() const noexcept {
        return content_tag_;
    }
    [[nodiscard]] std::uint16_t grid_dimension() const noexcept {
        return grid_dimension_;
    }
    [[nodiscard]] std::uint8_t levels() const noexcept {
        return levels_;
    }
    [[nodiscard]] std::uint32_t terrain_seed() const noexcept {
        return terrain_seed_;
    }
    [[nodiscard]] std::span<const ScenarioPlayerSlot> players() const noexcept {
        return players_;
    }
    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }
    [[nodiscard]] ScenarioDifficulty difficulty() const noexcept {
        return difficulty_;
    }
    [[nodiscard]] const std::string& description() const noexcept {
        return description_;
    }
    [[nodiscard]] std::span<const std::uint8_t> team_masks() const noexcept {
        return teams_;
    }
    [[nodiscard]] const std::optional<std::string>& loss_text() const noexcept {
        return loss_;
    }
    [[nodiscard]] const std::optional<std::string>& victory_text() const noexcept {
        return victory_;
    }
    [[nodiscard]] bool multiplayer() const noexcept {
        return multiplayer_;
    }

    [[nodiscard]] std::size_t body_offset() const noexcept {
        return body_offset_;
    }

    [[nodiscard]] std::size_t human_slots() const noexcept;
    [[nodiscard]] std::size_t alliance_count() const noexcept;

private:
    ScenarioHeader() = default;

    std::uint16_t version_        = 0;
    std::uint16_t content_tag_    = 0;
    std::uint16_t grid_dimension_ = 0;
    std::uint8_t levels_          = 0;
    std::uint32_t terrain_seed_   = 0;
    std::vector<ScenarioPlayerSlot> players_;
    std::string name_;
    ScenarioDifficulty difficulty_ = ScenarioDifficulty::easy;
    std::string description_;
    std::vector<std::uint8_t> teams_;
    std::optional<std::string> loss_;
    std::optional<std::string> victory_;
    bool multiplayer_        = false;
    std::size_t body_offset_ = 0;
};

}
