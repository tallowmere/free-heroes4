#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/asset_library.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/io/archive_chain.hpp"
#include "h4/io/png_writer.hpp"
#include "h4/model/alignment.hpp"
#include "h4/model/game_database.hpp"
#include "h4/model/scenario.hpp"
#include "h4/model/scenario_resolution.hpp"
#include "h4/model/scenario_setup.hpp"
#include "h4/rules/purse.hpp"
#include "h4/scene/viewer_shroud.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/boot/game_settings_view.hpp"
#include "h4/screens/boot/loading_view.hpp"
#include "h4/screens/boot/main_menu_view.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"
#include "h4/screens/shell/help_overlay.hpp"
#include "h4/screens/shell/resolution.hpp"
#include "tool.hpp"

namespace h4::tool {

int dump_map(Args args) {
    if (args.size() < 3 || args.size() > 8)
        return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if (!chain)
        return fail(chain.error());
    const assets::AssetLibrary library{*chain};
    auto scenario = args[1] == "tutorial" ? model::load_tutorial(library.game_map(model::kTutorialMap)) :
                                            model::Scenario::load(std::string{args[1]});
    if (!scenario)
        return fail(scenario.error());

    std::int32_t width = 1024;
    if (args.size() >= 6)
        std::from_chars(args[5].data(), args[5].data() + args[5].size(), width);
    std::optional<model::PlayerColour> viewer;
    if (args.size() >= 7) {
        std::int32_t colour = -1;
        std::from_chars(args[6].data(), args[6].data() + args[6].size(), colour);
        if (colour >= 0 && colour < static_cast<std::int32_t>(model::kPlayerColours))
            viewer = static_cast<model::PlayerColour>(colour);
    }
    const std::optional<model::Alignment> chosen = args.size() == 8 ? model::parse_alignment(args[7]) : std::nullopt;
    if (args.size() == 8 && !chosen.has_value())
        return usage();

    const auto database = model::GameDatabase::load(*chain);
    if (!database)
        return fail(database.error());
    model::ScenarioSetup setup = args[1] == "tutorial" ?
        model::ScenarioSetup::tutorial(scenario->summary(), false) :
        model::ScenarioSetup::fill(scenario->summary(), "Player", "Computer");
    if (chosen.has_value())
        for (std::size_t row = 0; row < setup.rows().size(); ++row)
            if (setup.rows()[row].colour == viewer)
                for (std::size_t step = 0; step <= model::kAllAlignments.size() && setup.rows()[row].alignment != chosen; ++step)
                    setup = setup.with_alignment_step(row, +1);
    *scenario = model::resolved_scenario(std::move(*scenario), setup, &*database);
    const scene::ViewerShroud shroud{library, scenario->map(), viewer, 0};
    const core::Size size{width, width == 800 ? 600 : (width == 1280 ? 1024 : 768)};
    auto view = screens::AdventureView::build(library, size);
    if (!view)
        return fail(view.error());
    (*view)->show_map(library, scenario->map(), shroud.plane());

    if (viewer)
        for (const model::MapActor* actor : scenario->map().owned_by(*viewer))
            if (actor->kind == model::ActorKind::hero || actor->kind == model::ActorKind::monster) {
                (*view)->map_window().bind({.selected = actor});
                std::println("selected {} at ({},{},{})", actor->name, actor->position.x, actor->position.y, actor->level);
                break;
            }
    const auto armies            = app::player_armies(scenario->map(), viewer);
    const model::MapActor* first = armies.empty() ? nullptr : armies.front();
    const auto heroes            = app::hero_list_entries(library, armies, &*database);
    const auto rings             = app::army_ring_entries(library, first);
    const auto towns = app::town_list_entries(library, scenario->map(), app::player_towns(scenario->map(), viewer));

    std::int32_t tile_x = scenario->map().dimension() / 2, tile_y = tile_x;
    if (args.size() >= 5) {
        std::from_chars(args[3].data(), args[3].data() + args[3].size(), tile_x);
        std::from_chars(args[4].data(), args[4].data() + args[4].size(), tile_y);
    }
    (*view)->map_window().look_at(tile_x, tile_y);
    const std::optional<std::size_t> ring = app::default_ring(first);
    (*view)->bind({
        .heroes        = heroes,
        .hero_selected = armies.empty() ? std::nullopt : std::optional<std::size_t>{0},
        .towns         = towns,
        .rings         = rings,
        .strip         = ring ? app::strip_entry(library, first, *ring, &*database) : screens::StatEntry{},
        .resources     = app::purse_amounts(rules::starting_purse(false, setup.difficulty())),
        .day           = "Day 1 of Week 1\nMonth 1",
    });

    graphics::Surface565 frame{size};
    const graphics::Canvas canvas{frame};
    (*view)->paint(canvas);
    if (auto written = io::write_png(std::string{args[2]}, frame.size(), frame.to_rgba()); !written)
        return fail(written.error());
    std::println("wrote {} at tile ({},{})", args[2], tile_x, tile_y);
    return EXIT_SUCCESS;
}

int screen(Args args) {
    if (args.size() != 3 && args.size() != 4)
        return usage();
    std::int32_t width = 1024, height = 0;
    const std::string_view spec = args.size() == 4 ? args[3] : std::string_view{"1024"};
    const auto* x               = std::ranges::find(spec, 'x');
    std::from_chars(spec.data(), x, width);
    if (x != spec.data() + spec.size())
        std::from_chars(x + 1, spec.data() + spec.size(), height);
    const core::Size size = height > 0 ? screens::logical_frame({width, height}) :
                                         screens::size_of(screens::variant_for_height(width == 800      ? 600 :
                                                                                          width == 1280 ? 1024 :
                                                                                                          768));
    constexpr std::int32_t kMaxSide = 16384;
    if (size.empty() || size.width > kMaxSide || size.height > kMaxSide)
        return usage();

    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if (!chain)
        return fail(chain.error());
    const assets::AssetLibrary library{*chain};
    graphics::Surface565 frame{size};
    const graphics::Canvas canvas{frame};

    if (const auto dialog = render_dialog(args[1], *chain, library, size, canvas)) {
        if (*dialog != EXIT_SUCCESS)
            return *dialog;
    } else if (args[1] == "adventure" || args[1] == "help") {
        const auto view = screens::AdventureView::build(library, size);
        if (!view)
            return fail(view.error());
        (*view)->bind({.day = "Day 1 of Week 1\nMonth 1"});
        (*view)->paint(canvas);
        if (args[1] == "help") {
            auto help = screens::HelpOverlay::build(library);
            if (!help)
                return fail(help.error());
            const core::Rect button = (*view)->button_rect(screens::AdventureButton::end_turn);
            const platform::InputEvent release = platform::events::PointerReleased{button.centre(), platform::PointerButton::secondary};
            help->settle(release, **view, size);
            help->paint(canvas);
        }
    } else {
        const auto resolution = screens::resolution_for_width(size.width);
        if (!resolution)
            return usage();
        const auto menu = screens::MainMenuView::build(library, screens::size_of(*resolution), screens::MainMenuModel{});
        if (!menu)
            return fail(menu.error());
        (*menu)->paint(canvas);
        if (args[1] == "loading") {
            const auto loading = screens::LoadingView::build(library, size, screens::LoadingModel{13});
            if (!loading)
                return fail(loading.error());
            (*loading)->paint(canvas);
        } else if (args[1] == "message") {
            auto message = screens::BasicDialog::build(
                library,
                size,
                screens::BasicDialogContent{
                    .text = "Ahead is a Level 1 Order Shrine.  Shrines provide you with a single "
                            "spell as long as you have the corresponding magic skill.  For "
                            "instance, this Shrine will teach you one Level 1 Order Magic spell "
                            "to Theodorus (who has the Order Magic skill) but not to Rissa (who "
                            "doesn't have the skill)."
                }
            );
            if (!message)
                return fail(message.error());
            (*message)->paint(canvas);
        } else if (args[1] == "question") {
            const auto frame_view = screens::AdventureView::build(library, size);
            if (!frame_view)
                return fail(frame_view.error());
            (*frame_view)->bind({.day = "Day 1 of Week 1\nMonth 1"});
            (*frame_view)->paint(canvas);
            const core::Rect button = (*frame_view)->button_rect(screens::AdventureButton::end_turn);
            auto question           = screens::BasicDialog::build(
                library,
                size,
                screens::BasicDialogContent{.text    = std::string{library.text("movement_left.misc")},
                                            .buttons = screens::DialogButtons::check_and_cross},
                screens::DialogPlacement::standing_on(button.centre())
            );
            if (!question)
                return fail(question.error());
            (*question)->paint(canvas);
        } else if (args[1] == "settings") {
            const auto settings = screens::GameSettingsView::build(
                library,
                size,
                screens::GameSettingsModel{.resolution       = 1,
                                           .checks           = {false, false, true, true, true, true},
                                           .slider_positions = {3, 3, 4, 100, 100},
                                           .slider_values    = {"1", "1", "2", "100", "100"},
                                           .port_checks      = {true}}
            );
            if (!settings)
                return fail(settings.error());
            (*settings)->paint(canvas);
        } else if (args[1] == "scroll") {
            const std::vector<std::string> labels{"Single Scenario", "Campaign", "Tutorial"};
            const core::Point anchor = (*menu)->action_art_rect(screens::MainMenuAction::new_game).origin();
            auto scroll = screens::ScrollMenuView::build(library, size, screens::ScrollMenuModel{anchor, labels});
            if (!scroll)
                return fail(scroll.error());
            core::Instant now;
            (*scroll)->advance(now);
            (*scroll)->advance(now += std::chrono::seconds{1});
            (*scroll)->paint(canvas);
        } else if (args[1] != "menu") {
            return usage();
        }
    }
    if (auto written = io::write_png(std::string{args[2]}, frame.size(), frame.to_rgba()); !written)
        return fail(written.error());
    std::println("wrote {}", args[2]);
    return EXIT_SUCCESS;
}

int screenshot(Args args) {
    if (args.size() != 2 && args.size() != 3)
        return usage();
    std::int32_t width = 1024;
    if (args.size() == 3)
        std::from_chars(args[2].data(), args[2].data() + args[2].size(), width);
    const auto resolution = screens::resolution_for_width(width);
    if (!resolution)
        return usage();

    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if (!chain)
        return fail(chain.error());
    const assets::AssetLibrary library{*chain};
    const auto view = screens::MainMenuView::build(library, screens::size_of(*resolution), screens::MainMenuModel{});
    if (!view)
        return fail(view.error());

    graphics::Surface565 frame{screens::size_of(*resolution)};
    (*view)->paint(graphics::Canvas{frame});
    if (auto written = io::write_png(std::string{args[1]}, frame.size(), frame.to_rgba()); !written)
        return fail(written.error());
    std::println("wrote {}", args[1]);
    return EXIT_SUCCESS;
}

}
