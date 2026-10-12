#pragma once

#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/resources.hpp"
#include "h4/rules/calendar.hpp"
#include "h4/rules/command.hpp"
#include "h4/rules/inventory.hpp"
#include "h4/rules/map_point.hpp"
#include "h4/rules/shrine.hpp"

namespace h4::rules {

struct ArmyPlace {
    std::uint32_t actor = 0;
    std::optional<PlayerId> owner;
    MapPoint at;
    std::int32_t day  = 0;
    std::int32_t left = 0;
};

struct Earner {
    std::uint32_t actor = 0;
    std::optional<PlayerId> owner;
    model::ResourceBundle daily;
};

struct Treasure {
    std::uint32_t actor = 0;
    model::ResourceBundle contents;
    bool taken = false;
    bool chest = false;
    std::optional<Artifact> artifact;
};

struct EventState {
    model::ResourceBundle gift;
    bool removes = false;
    bool gone    = false;
};

struct HeroState {
    std::uint32_t army = 0;
    std::uint8_t slot  = 0;
    std::array<std::optional<std::int32_t>, model::kMagicAlignments> school_ranks{};
    KnownSpells known;
    std::int64_t experience = 0;
    std::int32_t level      = 1;
    Inventory inventory;
};

struct ShrineState {
    std::uint32_t actor = 0;
    std::optional<model::SpellId> spell;
    std::optional<model::Alignment> school;
    std::int32_t level = 0;
};

struct ShrineLesson {
    std::vector<std::uint8_t> learners;
    bool already = false;
};

struct GameStart {
    std::uint8_t players = 1;
    std::vector<ArmyPlace> armies;
    std::vector<model::ResourceBundle> purses;
    std::vector<Earner> earners;
    std::vector<Treasure> treasures;
    std::vector<EventState> events;
    std::vector<HeroState> heroes;
    std::vector<ShrineState> shrines;
};

class GameState final {
public:
    [[nodiscard]] static core::Result<GameState> start(std::uint8_t player_count,
                                                       std::vector<ArmyPlace> armies             = {},
                                                       std::vector<model::ResourceBundle> purses = {},
                                                       std::vector<Earner> earners               = {},
                                                       std::vector<Treasure> treasures           = {},
                                                       std::vector<EventState> events            = {});
    [[nodiscard]] static core::Result<GameState> start(GameStart start);

    [[nodiscard]] Calendar calendar() const noexcept {
        return calendar_;
    }
    [[nodiscard]] PlayerId current_player() const noexcept {
        return current_;
    }
    [[nodiscard]] std::uint8_t player_count() const noexcept {
        return player_count_;
    }

    [[nodiscard]] std::span<const ArmyPlace> armies() const noexcept {
        return armies_;
    }
    [[nodiscard]] std::span<const Earner> earners() const noexcept {
        return earners_;
    }
    [[nodiscard]] const Treasure* treasure(std::uint32_t actor) const noexcept;
    [[nodiscard]] std::span<const EventState> events() const noexcept {
        return events_;
    }
    [[nodiscard]] std::span<const HeroState> heroes() const noexcept {
        return heroes_;
    }
    [[nodiscard]] auto heroes_of(std::uint32_t army) const noexcept {
        return heroes_ | std::views::filter([army](const HeroState& h) { return h.army == army; });
    }
    [[nodiscard]] bool chest_asks(std::uint32_t army, std::uint32_t chest) const noexcept;
    [[nodiscard]] const HeroState* carrier(std::uint32_t army) const noexcept;
    [[nodiscard]] const ShrineState* shrine(std::uint32_t actor) const noexcept;
    [[nodiscard]] ShrineLesson lesson(std::uint32_t army, std::uint32_t shrine) const;
    [[nodiscard]] model::ResourceBundle purse(PlayerId player) const noexcept;
    [[nodiscard]] const ArmyPlace* army(std::uint32_t actor) const noexcept;

    [[nodiscard]] core::Result<GameState> apply(const Command& command) const;

private:
    GameState(std::uint8_t player_count,
              Calendar calendar,
              PlayerId current,
              std::vector<ArmyPlace> armies,
              std::vector<model::ResourceBundle> purses,
              std::vector<Earner> earners,
              std::vector<Treasure> treasures,
              std::vector<EventState> events,
              std::vector<HeroState> heroes,
              std::vector<ShrineState> shrines) noexcept
    : player_count_{player_count}, calendar_{calendar}, current_{current}, armies_{std::move(armies)},
      purses_{std::move(purses)}, earners_{std::move(earners)}, treasures_{std::move(treasures)},
      events_{std::move(events)}, heroes_{std::move(heroes)}, shrines_{std::move(shrines)} {
    }

    [[nodiscard]] core::Result<GameState> end_turn(const commands::EndTurn& command) const;
    [[nodiscard]] core::Result<GameState> move_army(const commands::MoveArmy& command) const;
    [[nodiscard]] core::Result<GameState> visit(const commands::Visit& command) const;
    [[nodiscard]] core::Result<GameState> fire_event(const commands::FireEvent& command) const;
    [[nodiscard]] core::Result<const ArmyPlace*> own_army(PlayerId player, std::uint32_t army) const;
    [[nodiscard]] core::Result<GameState>
    take_treasure(const commands::Visit& command, const ArmyPlace& army, const Treasure& treasure) const;

    std::uint8_t player_count_;
    Calendar calendar_;
    PlayerId current_;
    std::vector<ArmyPlace> armies_;
    std::vector<model::ResourceBundle> purses_;
    std::vector<Earner> earners_;
    std::vector<Treasure> treasures_;
    std::vector<EventState> events_;
    std::vector<HeroState> heroes_;
    std::vector<ShrineState> shrines_;
};

}
