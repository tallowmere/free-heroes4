#pragma once

#include <optional>

#include "h4/core/signal.hpp"
#include "h4/scene/mini_map.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::model {
class Map;
}

namespace h4::screens {

class MapWindow;

class MiniMapWindow final : public ui::Widget {
public:
    MiniMapWindow(core::Rect rect, const MapWindow& map) noexcept : Widget{rect}, map_{&map} {
    }

    core::Signal<core::Point> pointed;

    void show_map(const assets::Assets& assets, const model::Map& map);

protected:
    void draw(const graphics::Canvas& canvas) const override;
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return image_.has_value();
    }
    void on_press(core::Point local) override {
        pointed.emit(map_pixel(local));
    }
    void on_drag(core::Point local) override {
        pointed.emit(map_pixel(local));
    }

private:
    [[nodiscard]] core::Point map_pixel(core::Point local) const noexcept;

    const MapWindow* map_;
    std::optional<scene::MiniMap> image_;
};

}
