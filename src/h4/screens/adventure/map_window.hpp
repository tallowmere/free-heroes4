#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "h4/core/signal.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/scene/map_hover.hpp"
#include "h4/scene/map_render.hpp"
#include "h4/ui/help.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Font;
}
namespace h4::model {
class Map;
struct MapActor;
class Shroud;
}

namespace h4::screens {

struct MapWindowModel {
    const model::MapActor* selected = nullptr;
    std::span<const scene::PathMark> path;
};

class MapWindow final : public ui::Widget {
public:
    MapWindow(core::Rect viewport, const formats::Font* floating_font);

    core::Signal<scene::MapHover> pointed;
    core::Signal<scene::MapHover> clicked;
    using Words = std::function<ui::Help(const scene::MapHover&)>;
    void set_words(Words words);

    void show_map(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen);
    [[nodiscard]] const scene::MapRenderer* renderer() const noexcept {
        return renderer_ ? &*renderer_ : nullptr;
    }

    void set_camera(core::Point camera) noexcept;
    [[nodiscard]] core::Point camera() const noexcept {
        return camera_;
    }
    void centre_on(core::Point pixel) noexcept;
    void look_at(std::int32_t x, std::int32_t y) noexcept;
    void look_at(const model::MapActor& actor) noexcept;
    [[nodiscard]] core::Point centre() const noexcept {
        return camera_ + core::Point{rect().width() / 2, rect().height() / 2};
    }
    void keep_in_sight(core::Point cell, std::int32_t level, bool bridge);
    [[nodiscard]] std::optional<core::Point> pin_in_root(core::Point cell, std::int32_t level) const;

    void bind(const MapWindowModel& model);
    void remove_object(std::uint32_t actor) noexcept;
    void pose_army(std::uint32_t actor, core::Point cell, std::int32_t level, const scene::ArmyPose& pose);
    [[nodiscard]] std::optional<scene::ArmyPose> army_pose(std::uint32_t actor) const noexcept;
    [[nodiscard]] std::optional<scene::ArmyFigure> army_figure(std::uint32_t actor) const;

    void float_text(core::Point cell, std::int32_t level, std::string text);
    [[nodiscard]] std::size_t floating_count() const noexcept {
        return floating_.size();
    }

    void set_clock(std::uint32_t clock_ms);

    void set_map_cursor(const cursor::CursorState& state) noexcept {
        map_cursor_ = state;
    }
    [[nodiscard]] std::optional<scene::MapHover> hover() const;

protected:
    void draw(const graphics::Canvas& canvas) const override;
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return renderer_.has_value();
    }
    void on_pointer_moved(core::Point local) override;
    void on_state_changed() override;
    void on_release(core::Point local) override {
        pointer_ = local;
    }
    void on_click() override;

private:
    struct Floating {
        core::Point at;
        std::string text;
        std::uint32_t born = 0;
    };

    [[nodiscard]] cursor::CursorState pointer_here(core::Point local) const noexcept;
    [[nodiscard]] scene::MapHover hover_at(core::Point local) const;

    const formats::Font* floating_font_;
    std::optional<scene::MapRenderer> renderer_;
    core::Point camera_{};
    std::optional<core::Point> pointer_;
    cursor::CursorState map_cursor_{};
    std::uint32_t clock_ms_ = 0;
    std::vector<Floating> floating_;
};

}
