#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {

struct EventAudience {
    std::uint8_t players = 0;
    bool humans          = false;
    bool computers       = false;

    [[nodiscard]] bool admits(std::uint8_t colour, bool human) const noexcept;
};

struct ScenarioEvent {
    std::string message;
    std::string name;
    std::vector<std::string> script_messages;
    EventAudience audience;
    bool removes = false;
    std::array<std::int32_t, 7> gift{};
    std::int32_t day    = 0;
    std::int32_t repeat = 0;

    [[nodiscard]] std::vector<std::string> words() const;
};

class ScenarioEvents {
public:
    [[nodiscard]] static core::Result<ScenarioEvents> parse(std::span<const std::byte> payload, std::size_t offset);

    [[nodiscard]] std::span<const ScenarioEvent> timed() const noexcept {
        return timed_;
    }
    [[nodiscard]] std::span<const ScenarioEvent> triggerable() const noexcept {
        return triggerable_;
    }
    [[nodiscard]] std::span<const ScenarioEvent> continuous() const noexcept {
        return continuous_;
    }
    [[nodiscard]] std::span<const ScenarioEvent> placed() const noexcept {
        return placed_;
    }

    [[nodiscard]] std::size_t end() const noexcept {
        return end_;
    }

private:
    ScenarioEvents() = default;

    std::vector<ScenarioEvent> timed_;
    std::vector<ScenarioEvent> triggerable_;
    std::vector<ScenarioEvent> continuous_;
    std::vector<ScenarioEvent> placed_;
    std::size_t end_ = 0;
};

inline constexpr std::int32_t kFirstDay = 1;

}
