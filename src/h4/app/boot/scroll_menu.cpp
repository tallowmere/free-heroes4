#include "h4/app/boot/scroll_menu.hpp"

#include <string>

#include "h4/assets/assets.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/shell/popup_host.hpp"

namespace h4::app {

core::Result<void> open_scroll_menu(ModalSlot<screens::ScrollMenuView>& slot,
                                    const assets::Assets& assets,
                                    core::Size frame,
                                    core::Point anchor,
                                    std::span<const std::string_view> keys,
                                    std::function<void(std::size_t)> chosen) {
    std::vector<std::string> labels;
    labels.reserve(keys.size());
    for (const std::string_view key : keys)
        labels.emplace_back(assets.text(key));
    auto scroll = screens::ScrollMenuView::build(assets, frame, screens::ScrollMenuModel{anchor, std::move(labels)});
    if (!scroll)
        return std::unexpected{scroll.error()};
    slot.open(std::move(*scroll),
              [&chosen](screens::ScrollMenuView& view, core::Connections& wiring) {
                  wiring.on(view.chosen, std::move(chosen));
              },
              {[&slot] { slot.close(); }, {}});
    return {};
}

}
