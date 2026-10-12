#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

#include "h4/app/modal_slot.hpp"
#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

template <class Entry> [[nodiscard]] std::vector<std::string_view> text_keys(std::span<const Entry> entries) {
    std::vector<std::string_view> keys;
    keys.reserve(entries.size());
    for (const Entry& entry : entries)
        keys.push_back(entry.text_key);
    return keys;
}

[[nodiscard]] core::Result<void> open_scroll_menu(ModalSlot<screens::ScrollMenuView>& slot,
                                                  const assets::Assets& assets,
                                                  core::Size frame,
                                                  core::Point anchor,
                                                  std::span<const std::string_view> keys,
                                                  std::function<void(std::size_t)> chosen);

}
