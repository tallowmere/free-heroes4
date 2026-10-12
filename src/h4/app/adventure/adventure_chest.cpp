#include <format>
#include <string>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/visit_words.hpp"
#include "h4/rules/treasure.hpp"

namespace h4::app {

void AdventureController::open_chest(ChestVisit visit) {
    if (!game_ || database_ == nullptr)
        return;
    const rules::Treasure* treasure = game_->treasure(visit.object);
    if (treasure == nullptr)
        return;
    screens::TreasureChestContent content = chest_words(
        *assets_, database_->texts(), scenario_.map().actors()[visit.object], treasure->contents[model::Resource::gold].value()
    );
    auto dialog = screens::TreasureChestDialog::build(*assets_, view_->frame_size(), std::move(content));
    if (report_.failed(dialog))
        return;
    chest_visit_ = visit;
    chest_.open(std::move(*dialog),
                [this](screens::TreasureChestDialog& chest, core::Connections& wiring) {
                    wiring.on(chest.chosen, [this](screens::TreasureChestDialog::Choice choice) { close_chest(choice); });
                },
                {[this] { close_chest(chest_.get()->choice()); }, [this] { close_chest(chest_.get()->choice()); }});
}

void AdventureController::close_chest(screens::TreasureChestDialog::Choice choice) {
    chest_.close();
    if (!chest_visit_ || !game_ || !me_)
        return;
    const ChestVisit visit = *chest_visit_;
    chest_visit_.reset();

    std::vector<std::int32_t> before;
    for (const rules::HeroState& hero : game_->heroes())
        before.push_back(hero.level);
    rules::commands::Visit command{*me_, visit.army, visit.object, visit.cost};
    command.experience = choice == screens::TreasureChestDialog::Choice::experience;
    if (!apply(command))
        return;
    take_object(visit.object);
    const auto heroes = game_->heroes();
    for (std::size_t i = 0; i < heroes.size() && i < before.size(); ++i)
        if (heroes[i].level > before[i]) {
            const model::MapActor& army = scenario_.map().actors()[heroes[i].army];
            const std::string_view name = heroes[i].slot < army.army.slots.size() ? army.army.slots[heroes[i].slot].hero.name :
                                                                                    std::string_view{};
            report_.not_ported(std::format("{} reaching level {}", name, heroes[i].level));
        }
    show_purse();
    select(selected_);
    show_heroes();
}

void AdventureController::found_artifact(std::uint32_t object, const rules::PathField::PathStep& step, const rules::Artifact& artifact) {
    if (database_ == nullptr)
        return;
    const std::optional<model::ArtifactId> id = model::artifact_id_of(artifact.instance.id);
    if (const model::ArtifactType* type = id ? database_->artifacts().find(*id) : nullptr; type != nullptr)
        notice_over(step, artifact_words(*assets_, database_->texts(), scenario_.map().actors()[object], *type));
}

}
