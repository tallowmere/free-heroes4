#include <algorithm>
#include <format>
#include <vector>

#include "h4/rules/experience.hpp"
#include "h4/rules/game_state.hpp"
#include "h4/rules/treasure.hpp"

namespace h4::rules {

core::Result<GameState> GameState::take_treasure(const commands::Visit& command, const ArmyPlace& army, const Treasure& treasure) const {
    if (treasure.taken)
        return core::fail(core::ErrorCode::invalid_argument, std::format("actor {} is already taken", command.object));
    if (command.experience && !chest_asks(command.army, command.object))
        return core::fail(core::ErrorCode::invalid_argument, std::format("actor {} offers no experience", command.object));
    const HeroState* carries = treasure.artifact.has_value() ? carrier(command.army) : nullptr;
    if (treasure.artifact.has_value() && carries == nullptr)
        return core::fail(core::ErrorCode::unsupported, "an artifact for an army with no hero");

    GameState next = *this;
    next.armies_[static_cast<std::size_t>(&army - armies_.data())].left -= command.cost;
    if (carries != nullptr) {
        HeroState& hero = next.heroes_[static_cast<std::size_t>(carries - heroes_.data())];
        hero.inventory  = receive(std::move(hero.inventory), *treasure.artifact);
    } else if (command.experience) {
        std::vector<HeroState*> heroes;
        std::vector<std::int32_t> levels;
        for (HeroState& hero : next.heroes_)
            if (hero.army == command.army && hero.level < kMaxLevel) {
                heroes.push_back(&hero);
                levels.push_back(hero.level);
            }
        const auto shares = split_award(chest_experience(treasure.contents[model::Resource::gold].value()), levels, true);
        for (std::size_t i = 0; i < heroes.size(); ++i) {
            heroes[i]->experience += shares[i];
            heroes[i]->level = std::max(heroes[i]->level, level_for_experience(heroes[i]->experience));
        }
    } else {
        model::ResourceBundle& purse = next.purses_[command.player.value()];
        purse                        = purse + treasure.contents;
    }
    next.treasures_[static_cast<std::size_t>(&treasure - treasures_.data())].taken = true;
    return next;
}

}
