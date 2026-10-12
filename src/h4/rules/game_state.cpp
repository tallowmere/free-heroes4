#include "h4/rules/game_state.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <utility>

#include "h4/rules/experience.hpp"
#include "h4/rules/treasure.hpp"

namespace h4::rules {

namespace {

constexpr std::uint8_t kMaxPlayers = 8;

template <class T> [[nodiscard]] const T* find_by_actor(const std::vector<T>& list, std::uint32_t actor) noexcept {
    const auto it = std::ranges::lower_bound(list, actor, {}, &T::actor);
    return it != list.end() && it->actor == actor ? &*it : nullptr;
}

}

core::Result<GameState> GameState::start(std::uint8_t player_count,
                                         std::vector<ArmyPlace> armies,
                                         std::vector<model::ResourceBundle> purses,
                                         std::vector<Earner> earners,
                                         std::vector<Treasure> treasures,
                                         std::vector<EventState> events) {
    return start(GameStart{
        player_count, std::move(armies), std::move(purses), std::move(earners), std::move(treasures), std::move(events), {}, {}
    });
}

core::Result<GameState> GameState::start(GameStart start) {
    const std::uint8_t player_count = start.players;
    if (player_count == 0 || player_count > kMaxPlayers)
        return core::fail(core::ErrorCode::invalid_argument, std::format("{} players; a game has 1 to {}", player_count, kMaxPlayers));
    std::vector<ArmyPlace>& armies = start.armies;
    std::ranges::sort(armies, {}, &ArmyPlace::actor);
    if (std::ranges::adjacent_find(armies, {}, &ArmyPlace::actor) != armies.end())
        return core::fail(core::ErrorCode::invalid_argument, "an actor placed as two armies");
    for (ArmyPlace& army : armies) {
        if (army.owner.has_value() && army.owner->value() >= player_count)
            return core::fail(core::ErrorCode::invalid_argument, std::format("an army of player {}", army.owner->value()));
        army.left = army.day;
    }
    if (start.purses.size() > player_count)
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("{} purses for {} players", start.purses.size(), player_count));
    start.purses.resize(player_count);
    for (const Earner& earner : start.earners)
        if (earner.owner.has_value() && earner.owner->value() >= player_count)
            return core::fail(core::ErrorCode::invalid_argument, std::format("an earner of player {}", earner.owner->value()));
    std::ranges::sort(start.treasures, {}, &Treasure::actor);
    std::ranges::sort(start.shrines, {}, &ShrineState::actor);
    return GameState{player_count,
                     Calendar{},
                     PlayerId{0},
                     std::move(armies),
                     std::move(start.purses),
                     std::move(start.earners),
                     std::move(start.treasures),
                     std::move(start.events),
                     std::move(start.heroes),
                     std::move(start.shrines)};
}

const ShrineState* GameState::shrine(std::uint32_t actor) const noexcept {
    return find_by_actor(shrines_, actor);
}

ShrineLesson GameState::lesson(std::uint32_t army, std::uint32_t actor) const {
    ShrineLesson out;
    const ShrineState* const at = shrine(actor);
    if (at == nullptr || !at->spell || !at->school)
        return out;
    for (const HeroState& hero : heroes_of(army)) {
        if (hero.known.test(at->spell->value()))
            out.already = true;
        else if (may_learn(hero.school_ranks[std::to_underlying(*at->school)], at->level))
            out.learners.push_back(hero.slot);
    }
    return out;
}

const HeroState* GameState::carrier(std::uint32_t army) const noexcept {
    const HeroState* best = nullptr;
    for (const HeroState& hero : heroes_of(army))
        if (best == nullptr || hero.level > best->level || (hero.level == best->level && hero.slot < best->slot))
            best = &hero;
    return best;
}

bool GameState::chest_asks(std::uint32_t army, std::uint32_t chest) const noexcept {
    const Treasure* at = treasure(chest);
    if (at == nullptr || !at->chest || at->contents[model::Resource::gold].value() == 0)
        return false;
    return std::ranges::any_of(heroes_of(army), [](const HeroState& hero) { return hero.level < kMaxLevel; });
}

const ArmyPlace* GameState::army(std::uint32_t actor) const noexcept {
    return find_by_actor(armies_, actor);
}

const Treasure* GameState::treasure(std::uint32_t actor) const noexcept {
    return find_by_actor(treasures_, actor);
}

model::ResourceBundle GameState::purse(PlayerId player) const noexcept {
    return player.value() < purses_.size() ? purses_[player.value()] : model::ResourceBundle{};
}

core::Result<GameState> GameState::apply(const Command& command) const {
    struct Dispatch {
        const GameState* state;
        core::Result<GameState> operator()(const commands::EndTurn& c) const {
            return state->end_turn(c);
        }
        core::Result<GameState> operator()(const commands::MoveArmy& c) const {
            return state->move_army(c);
        }
        core::Result<GameState> operator()(const commands::Visit& c) const {
            return state->visit(c);
        }
        core::Result<GameState> operator()(const commands::FireEvent& c) const {
            return state->fire_event(c);
        }
    };
    return std::visit(Dispatch{this}, command);
}

core::Result<GameState> GameState::end_turn(const commands::EndTurn& command) const {
    if (command.player != current_)
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("player {} ended a turn that is player "
                                      "{}'s",
                                      command.player.value(),
                                      current_.value()));
    GameState next    = *this;
    const auto player = static_cast<std::uint8_t>(current_.value() + 1);
    if (player < player_count_) {
        next.current_ = PlayerId{player};
        return next;
    }
    next.current_  = PlayerId{0};
    next.calendar_ = calendar_.next();
    for (ArmyPlace& army : next.armies_)
        army.left = army.day;
    for (const Earner& earner : earners_)
        if (earner.owner.has_value()) {
            model::ResourceBundle& purse = next.purses_[earner.owner->value()];
            purse                        = purse + earner.daily;
        }
    return next;
}

core::Result<const ArmyPlace*> GameState::own_army(PlayerId player, std::uint32_t actor) const {
    if (player != current_)
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("player {} moved on player {}'s turn", player.value(), current_.value()));
    const ArmyPlace* army = this->army(actor);
    if (army == nullptr)
        return core::fail(core::ErrorCode::invalid_argument, std::format("actor {} is not an army", actor));
    if (army->owner != player)
        return core::fail(core::ErrorCode::invalid_argument, std::format("actor {} is not player {}'s", actor, player.value()));
    return army;
}

core::Result<GameState> GameState::move_army(const commands::MoveArmy& command) const {
    const auto own = own_army(command.player, command.army);
    if (!own)
        return std::unexpected{own.error()};
    const ArmyPlace* army = *own;
    const MapPoint& from  = army->at;
    const MapPoint& to    = command.to;
    if (std::abs(to.x - from.x) > 1 || std::abs(to.y - from.y) > 1 || to == from || std::abs(to.level - from.level) > 1)
        return core::fail(core::ErrorCode::invalid_argument, "a step to a place not beside it");
    if (command.cost <= 0 || command.cost > army->left)
        return core::fail(core::ErrorCode::invalid_argument, std::format("a step of {} with {} left", command.cost, army->left));

    GameState next   = *this;
    ArmyPlace& moved = next.armies_[static_cast<std::size_t>(army - armies_.data())];
    moved.at         = to;
    moved.left -= command.cost;
    return next;
}

core::Result<GameState> GameState::visit(const commands::Visit& command) const {
    const auto own = own_army(command.player, command.army);
    if (!own)
        return std::unexpected{own.error()};
    const ArmyPlace* army = *own;
    if (command.cost < 0 || command.cost > army->left)
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("a visit costing {} with {} left", command.cost, army->left));
    if (const ShrineState* at = shrine(command.object); at != nullptr) {
        const ShrineLesson taught = lesson(command.army, command.object);
        GameState next            = *this;
        next.armies_[static_cast<std::size_t>(army - armies_.data())].left -= command.cost;
        for (HeroState& hero : next.heroes_)
            if (hero.army == command.army && std::ranges::find(taught.learners, hero.slot) != taught.learners.end())
                hero.known.set(at->spell->value());
        return next;
    }
    const Treasure* treasure = this->treasure(command.object);
    if (treasure == nullptr)
        return core::fail(core::ErrorCode::unsupported, std::format("no visit for actor {}", command.object));
    return take_treasure(command, *army, *treasure);
}

core::Result<GameState> GameState::fire_event(const commands::FireEvent& command) const {
    if (command.player != current_)
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("an event fired for player {} on "
                                      "player {}'s turn",
                                      command.player.value(),
                                      current_.value()));
    if (command.event >= events_.size() || events_[command.event].gone)
        return core::fail(core::ErrorCode::invalid_argument, std::format("event {} is not on the map", command.event));
    GameState next               = *this;
    EventState& event            = next.events_[command.event];
    model::ResourceBundle& purse = next.purses_[command.player.value()];
    purse                        = purse + event.gift;
    if (event.removes)
        event.gone = true;
    return next;
}

}
