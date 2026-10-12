#pragma once

#include <cstdint>
#include <memory>

#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget.hpp"

namespace h4::formats {
class Layer;
}
namespace h4::ui {
class Meter;
class Widget;
}

namespace h4::screens {

struct LoadingModel {
    std::int32_t percent = 0;
};

class LoadingView final : public screens::ModalPopup {
    friend ViewFactory;
    explicit LoadingView(core::Size frame, LoadingModel model);

public:
    [[nodiscard]] static core::Result<std::unique_ptr<LoadingView>>
    build(const assets::Assets& assets, core::Size frame, LoadingModel model);

    void bind(const LoadingModel& model);

    [[nodiscard]] core::Rect bar_rect() const noexcept;
    [[nodiscard]] core::Rect filled_rect() const noexcept;
    [[nodiscard]] std::int32_t progress() const noexcept {
        return model_.percent;
    }

private:
    core::Result<void> compose(const assets::Assets& assets);

    LoadingModel model_;
    ui::Widget* bar_                  = nullptr;
    ui::Meter* fill_                  = nullptr;
    const formats::Layer* fill_layer_ = nullptr;
};

}
