#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/asset_library.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/io/archive_chain.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/scenario.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/adventure/treasure_chest_dialog.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "tool.hpp"

namespace h4::tool {

std::optional<int> render_dialog(std::string_view which,
                                 const io::ArchiveChain& chain,
                                 const assets::AssetLibrary& library,
                                 core::Size size,
                                 const graphics::Canvas& canvas) {
    if (which == "chest") {
        const auto database = model::GameDatabase::load(chain);
        if (!database)
            return fail(database.error());
        const auto row = [&](std::string_view keyword) {
            return std::string{database->texts().text("treasure", "treasure_chest", {}, keyword)};
        };
        auto chest = screens::TreasureChestDialog::build(
            library,
            size,
            screens::TreasureChestContent{row("name"),
                                          row("Initial"),
                                          model::substituted(row("gold"), {{"%material", "1500 gold"}}),
                                          model::substituted(row("experience"), {{"%experience", "1000"}})}
        );
        if (!chest)
            return fail(chest.error());
        (*chest)->paint(canvas);
        return EXIT_SUCCESS;
    }
    if (which == "artifact") {
        const auto database = model::GameDatabase::load(chain);
        if (!database)
            return fail(database.error());
        const model::ArtifactType* crystal = nullptr;
        for (const model::ArtifactType& type : database->artifacts().all())
            if (type.name == "Viewing Crystal")
                crystal = &type;
        if (crystal == nullptr)
            return fail(core::Error{core::ErrorCode::not_found, "crystal"});
        const std::string words = model::substituted(database->texts().text("treasure", "treasure_chest", {}, "Artifact"),
                                                     {{"%an_artifact", crystal->article}});
        screens::DialogItem item{{}, crystal->name, {.balloon = crystal->name}};
        if (const formats::Layer* icon = app::artifact_icon(library, *crystal))
            item.pieces.push_back({icon, {}});
        auto dialog = screens::BasicDialog::build(
            library,
            size,
            screens::BasicDialogContent{
                std::string{database->texts().text("treasure", "treasure_chest", {}, "name")}, words, true, {item}, screens::DialogButtons::check
            }
        );
        if (!dialog)
            return fail(dialog.error());
        (*dialog)->paint(canvas);
        return EXIT_SUCCESS;
    }
    if (which.starts_with("army")) {
        int slot = 0;
        if (which.size() > 4)
            slot = which[4] - '0';
        const auto database = model::GameDatabase::load(chain);
        if (!database)
            return fail(database.error());
        auto scenario = model::load_tutorial(library.game_map(model::kTutorialMap));
        if (!scenario)
            return fail(scenario.error());
        auto setup = model::ScenarioSetup::tutorial(scenario->summary(), true);
        auto view  = screens::AdventureView::build(library, size);
        if (!view)
            return fail(view.error());
        std::vector<std::string> reports;
        app::AdventureController game{std::move(*view),
                                      library,
                                      app::resolve_and_open(std::move(*scenario), std::move(setup), &*database).value(),
                                      {},
                                      [&](std::string_view m) { reports.emplace_back(m); },
                                      &*database};
        while (game.message_open())
            game.on_key(platform::Key::escape);
        const core::Rect ring = game.frame().army_rings().ring_rect(static_cast<std::size_t>(std::clamp(slot, 0, 6)));
        const core::Point at  = ring.centre();
        game.view().handle(platform::events::PointerPressed{at});
        game.view().handle(platform::events::PointerReleased{at});
        game.view().handle(platform::events::PointerPressed{at, platform::PointerButton::primary, true});
        game.view().handle(platform::events::PointerReleased{at});
        game.view().paint(canvas);
        return EXIT_SUCCESS;
    }
    return std::nullopt;
}

}
