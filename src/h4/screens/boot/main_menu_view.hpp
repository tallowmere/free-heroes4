#pragma once

#include <memory>

#include "h4/core/signal.hpp"
#include "h4/screens/boot/menu_banners.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::screens {

struct MainMenuModel {
    bool actions_enabled = true;
};

class MainMenuView final : public screens::WidgetView<screens::View> {
    friend ViewFactory;
    explicit MainMenuView(core::Size frame, MainMenuModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<MainMenuView>>
    build(const assets::Assets& assets, core::Size frame, MainMenuModel model);

    void bind(const MainMenuModel& model);

    [[nodiscard]] core::Rect action_rect(MainMenuAction which) const noexcept;
    [[nodiscard]] core::Rect action_art_rect(MainMenuAction which) const noexcept;

    core::Signal<MainMenuAction> action;

private:
    core::Result<void> compose(const assets::Assets& assets);

    MainMenuModel model_;
    MenuBanners* banners_ = nullptr;
    core::Connections connections_;
};

}
