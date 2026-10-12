#pragma once

#include <memory>
#include <string>
#include <vector>

#include "h4/core/clock.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/boot/scroll_menu.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::screens {

struct ScrollMenuModel {
    core::Point anchor;
    std::vector<std::string> labels;
};

class ScrollMenuView final : public screens::WidgetView<screens::Popup> {
    friend ViewFactory;
    explicit ScrollMenuView(core::Size frame, ScrollMenuModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<ScrollMenuView>>
    build(const assets::Assets& assets, core::Size frame, ScrollMenuModel model);

    [[nodiscard]] std::size_t entry_count() const noexcept;
    [[nodiscard]] core::Rect entry_rect(std::size_t index) const noexcept;
    [[nodiscard]] bool entry_washed(std::size_t index) const noexcept;
    [[nodiscard]] bool unrolled() const noexcept;
    [[nodiscard]] core::Rect scroll_rect() const noexcept;
    [[nodiscard]] core::Rect body_rect() const noexcept;
    [[nodiscard]] core::Rect border_rect() const noexcept;
    [[nodiscard]] core::Rect curl_rect() const noexcept;

    core::Signal<std::size_t> chosen;

    [[nodiscard]] bool holds(core::Point point) const override;
    [[nodiscard]] bool covers(core::Point point) const override;
    void advance(core::Instant now) override;

private:
    core::Result<void> compose(const assets::Assets& assets);

    ScrollMenuModel model_;
    ScrollMenu* menu_ = nullptr;
    core::Connections connections_;
};

}
