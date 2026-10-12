#include "h4/screens/boot/menu_banners.hpp"

#include <format>
#include <string>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/label.hpp"

namespace h4::screens {

MenuBanners::MenuBanners(Builder& builder, std::span<const MenuBanner, kMainMenuActions> banners)
: Grouping{builder, "0x76DC10 installs help on each banner alone"} {
    Builder in = builder.inside(*this);
    for (std::size_t i = 0; i < banners.size(); ++i) {
        const MenuBanner& banner = banners[i];
        const auto help          = in.help(banner.help_item);
        auto& button = in.add<ui::Button>(help, core::Point{}, in.faces(banner.faces_base));
        button.set_name(std::string{banner.faces_base});
        buttons_[i] = &button;
        connections_.relay(button.clicked, clicked, static_cast<MainMenuAction>(i));

        const std::string text_name = std::format("{}_Text", banner.faces_base);
        if (in.find(text_name) != nullptr)
            in.label(text_name, in.text(banner.text_key), ui::kEngravedCentred, help).set_name(text_name);
    }
}

void MenuBanners::set_enabled_all(bool enabled) {
    for (ui::Button* button : buttons_)
        button->set_enabled(enabled);
}

core::Rect MenuBanners::button_rect(MainMenuAction action) const noexcept {
    return button(action).rect_in_root();
}

core::Rect MenuBanners::art_rect(MainMenuAction action) const noexcept {
    const ui::Button& b = button(action);
    return b.art_bounds().translated(b.rect_in_root().origin() - b.rect().origin());
}

}
