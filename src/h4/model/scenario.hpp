#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/formats/scenario/events.hpp"
#include "h4/model/map.hpp"
#include "h4/model/resources.hpp"
#include "h4/model/scenario_summary.hpp"

namespace h4::model {

struct ScenarioMessage {
    std::string text;
    std::int32_t day    = 1;
    std::int32_t repeat = 0;
    formats::EventAudience audience;

    [[nodiscard]] bool admits(PlayerColour colour, bool human) const noexcept;
    [[nodiscard]] bool fires_on(std::int32_t day) const noexcept;
};

struct PlacedEvent {
    std::string name;
    std::vector<std::string> words;
    ResourceBundle gift;
    formats::EventAudience audience;
    bool removes = false;

    [[nodiscard]] bool admits(PlayerColour colour, bool human) const noexcept;
};

class Scenario {
public:
    [[nodiscard]] static core::Result<Scenario> load(const std::filesystem::path& file);
    [[nodiscard]] static core::Result<Scenario> parse(std::span<const std::byte> file, const std::filesystem::path& path);

    [[nodiscard]] const ScenarioSummary& summary() const noexcept {
        return summary_;
    }
    [[nodiscard]] std::uint16_t grid_dimension() const noexcept {
        return grid_;
    }
    [[nodiscard]] std::uint8_t levels() const noexcept {
        return levels_;
    }

    [[nodiscard]] std::vector<std::string> messages_on(std::int32_t day, PlayerColour colour, bool human) const;
    [[nodiscard]] std::vector<std::string> opening_messages(PlayerColour colour, bool human) const {
        return messages_on(1, colour, human);
    }

    [[nodiscard]] const std::vector<ScenarioMessage>& timed_messages() const noexcept {
        return timed_;
    }
    [[nodiscard]] const std::vector<PlacedEvent>& placed_events() const noexcept {
        return placed_;
    }

    [[nodiscard]] const Map& map() const noexcept {
        return *map_;
    }

    void resolve_random_objects(const RandomResolution& choices) {
        if (map_.has_value())
            map_->resolve_random_objects(choices);
    }
    void resolve_random_objects(const RandomResolution& choices, Roller& stream) {
        if (map_.has_value())
            map_->resolve_random_objects(choices, stream);
    }

private:
    Scenario() = default;

    ScenarioSummary summary_;
    std::vector<ScenarioMessage> timed_;
    std::vector<PlacedEvent> placed_;
    std::optional<Map> map_;
    std::uint16_t grid_  = 0;
    std::uint8_t levels_ = 0;
};

[[nodiscard]] core::Result<ScenarioSummary> read_scenario_summary(const std::filesystem::path& file);
[[nodiscard]] core::Result<ScenarioSummary> read_scenario_summary(std::span<const std::byte> file, const std::filesystem::path& path);

inline constexpr std::string_view kTutorialMap  = "single.Tutorial";
inline constexpr std::string_view kTutorialPath = "maps/tutorial.h4c";
[[nodiscard]] core::Result<ScenarioSummary> read_tutorial_summary(std::span<const std::byte> resource);
[[nodiscard]] core::Result<Scenario> load_tutorial(std::span<const std::byte> resource);

}
