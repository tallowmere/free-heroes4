#include "h4/screens/boot/main_menu_view.hpp"

#include <array>
#include <format>
#include <string>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/color.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/shell/resolution.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/painting.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kHelpWindow = "main_menu";

constexpr std::array<MenuBanner, kMainMenuActions> kBanners{{
    {"New_Game", "new_game.main_menu", "new_game"},
    {"Load_Game", "load_game.main_menu", "load_game"},
    {"Options", "options.main_menu", "options"},
    {"Quit", "quit.main_menu", "quit"},
    {"Network", "multiplayer.main_menu", "multiplayer"},
}};

constexpr std::array<std::string_view, 3> kCopyrightKeys{"x2_copyright.main_menu", "storm_copyright.main_menu", "copyright.main_menu"};

std::int32_t copyright_font_size(Resolution r) noexcept {
    switch (r) {
    case Resolution::r800x600: return 10;
    case Resolution::r1024x768: return 12;
    case Resolution::r1280x1024: return 14;
    }
    return 12;
}

}

core::Result<std::unique_ptr<MainMenuView>> MainMenuView::build(const assets::Assets& assets, core::Size frame, MainMenuModel model) {
    return compose_view<MainMenuView>(assets, frame, model);
}

MainMenuView::MainMenuView(core::Size frame, MainMenuModel model) : WidgetView{frame}, model_{model} {
}

core::Result<void> MainMenuView::compose(const assets::Assets& assets) {
    const Resolution resolution          = resolution_for_width(frame_size().width).value_or(Resolution::r800x600);
    const std::string variant            = std::format("menu.main.{}", variant_suffix(resolution));
    const graphics::Surface565* backdrop = assets.painting(variant);
    if (backdrop == nullptr)
        return core::fail(core::ErrorCode::not_found, std::format("main menu: bitmap_raw '{}' missing", variant));
    auto opened = Builder::open(tree().root(), assets, {variant, kHelpWindow, "main menu"});
    if (!opened)
        return std::unexpected{opened.error()};
    Builder& menu = *opened;

    const auto quiet = HelpDecision::silent("0x76DC10 helps the banners alone");
    menu.add<ui::Painting>(quiet, *backdrop, core::Point{});

    banners_ = &menu.add<MenuBanners>(HelpDecision::from_parent(), menu, kBanners);
    connections_.relay(banners_->clicked, action);

    if (const formats::Layer* guide = menu.find("Copywright_Text")) {
        std::string words;
        for (const std::string_view key : kCopyrightKeys)
            if (words = menu.text(key); !words.empty())
                break;
        if (!words.empty())
            menu.label(guide->bounds(),
                       std::move(words),
                       ui::LabelStyle{{graphics::colors::white, graphics::colors::black}, {}},
                       quiet,
                       copyright_font_size(resolution));
    }

    bind(model_);
    return menu.finish();
}

void MainMenuView::bind(const MainMenuModel& model) {
    model_ = model;
    banners_->set_enabled_all(model_.actions_enabled);
}

core::Rect MainMenuView::action_rect(MainMenuAction which) const noexcept {
    return banners_->button_rect(which);
}

core::Rect MainMenuView::action_art_rect(MainMenuAction which) const noexcept {
    return banners_->art_rect(which);
}

}
