#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "h4/model/alignment.hpp"
#include "h4/model/scenario_summary.hpp"

namespace h4::model {

struct SetupRow {
    PlayerColour colour = PlayerColour::red;
    std::optional<Alignment> alignment;
    bool human = false;
    std::string name;
};

class ScenarioSetup {
public:
    [[nodiscard]] static ScenarioSetup fill(const ScenarioSummary& scenario, std::string player_name, std::string computer_name);

    [[nodiscard]] static ScenarioSetup tutorial(const ScenarioSummary& scenario, bool guards_move);

    [[nodiscard]] const std::vector<SetupRow>& rows() const noexcept {
        return rows_;
    }
    [[nodiscard]] MapDifficulty difficulty() const noexcept {
        return difficulty_;
    }
    [[nodiscard]] bool guards_move() const noexcept {
        return guards_move_;
    }

    [[nodiscard]] bool human_capable(std::size_t row) const noexcept;
    [[nodiscard]] std::uint8_t allowed_alignments(std::size_t row) const noexcept;
    [[nodiscard]] std::size_t team(std::size_t row) const noexcept;
    [[nodiscard]] bool colour_arrows_enabled(std::size_t row) const noexcept;
    [[nodiscard]] bool alignment_arrows_enabled(std::size_t row) const noexcept;

    [[nodiscard]] ScenarioSetup with_alignment_step(std::size_t row, int delta) const;
    [[nodiscard]] ScenarioSetup with_colour_step(std::size_t row, int delta) const;
    [[nodiscard]] ScenarioSetup with_human(std::size_t row, bool human) const;
    [[nodiscard]] ScenarioSetup with_difficulty_step(int delta) const;
    [[nodiscard]] ScenarioSetup with_guards_move(bool move) const;

private:
    static constexpr std::size_t kNoTeam = 0xFF;

    struct Slot {
        bool available          = false;
        bool human_capable      = false;
        std::uint8_t alignments = 0;
        std::size_t team        = kNoTeam;
    };

    ScenarioSetup() = default;

    [[nodiscard]] const Slot& slot_of(std::size_t row) const noexcept;
    void make_human(std::size_t row);

    std::array<Slot, kPlayerColours> slots_{};
    std::vector<SetupRow> rows_;
    std::vector<std::size_t> team_order_;
    std::vector<std::string> pool_;
    std::string computer_name_;
    std::size_t human_slots_  = 0;
    MapDifficulty difficulty_ = MapDifficulty::normal;
    bool guards_move_         = false;
};

}
