#include "h4/screens/adventure/map_window.hpp"

#include <algorithm>
#include <utility>

#include "h4/formats/font.hpp"
#include "h4/graphics/canvas.hpp"
#include "h4/model/map.hpp"

namespace h4::screens {

namespace {
constexpr std::uint32_t kFloatMs     = 3000;
constexpr std::uint32_t kFloatStepMs = 100;
}

MapWindow::MapWindow(core::Rect viewport, const formats::Font* floating_font)
: Widget{viewport}, floating_font_{floating_font} {
    set_pointer([this](core::Point local) { return pointer_here(local); });
    popup_at_pointer();
}

void MapWindow::show_map(const assets::Assets& assets, const model::Map& map, const model::Shroud* seen) {
    renderer_.emplace(assets, map, seen);
}

void MapWindow::set_camera(core::Point camera) noexcept {
    if (renderer_) {
        const core::Size box  = renderer_->extent();
        const core::Size view = rect().size();
        camera.x              = std::max(camera.x, -scene::kCameraMargin);
        camera.y              = std::max(camera.y, -(scene::kCameraMargin + scene::kRaiseAllowance));
        if (box.width + scene::kCameraMargin < camera.x + view.width)
            camera.x = box.width - view.width + scene::kCameraMargin;
        if (box.height + scene::kCameraMargin < camera.y + view.height)
            camera.y = box.height - view.height + scene::kCameraMargin;
    }
    camera_ = camera;
}

void MapWindow::centre_on(core::Point pixel) noexcept {
    set_camera({pixel.x - rect().width() / 2, pixel.y - rect().height() / 2});
}

void MapWindow::look_at(std::int32_t x, std::int32_t y) noexcept {
    if (renderer_)
        centre_on(renderer_->pin(x, y));
}

void MapWindow::look_at(const model::MapActor& actor) noexcept {
    if (renderer_)
        centre_on(renderer_->centre_of(actor));
}

void MapWindow::keep_in_sight(core::Point cell, std::int32_t level, bool bridge) {
    if (!renderer_)
        return;
    const core::Point pin = renderer_->army_pin(cell, level, bridge);
    if (!core::Rect{camera_, rect().size()}.contains(pin))
        centre_on(pin);
}

std::optional<core::Point> MapWindow::pin_in_root(core::Point cell, std::int32_t level) const {
    if (!renderer_)
        return std::nullopt;
    return renderer_->army_pin(cell, level) - camera_ + rect_in_root().origin();
}

void MapWindow::bind(const MapWindowModel& model) {
    if (!renderer_)
        return;
    renderer_->select_army(model.selected);
    renderer_->show_path(model.path);
}

void MapWindow::remove_object(std::uint32_t actor) noexcept {
    if (renderer_)
        renderer_->remove_object(actor);
}

void MapWindow::pose_army(std::uint32_t actor, core::Point cell, std::int32_t level, const scene::ArmyPose& pose) {
    if (renderer_)
        renderer_->pose_army(actor, cell, level, pose);
}

std::optional<scene::ArmyPose> MapWindow::army_pose(std::uint32_t actor) const noexcept {
    return renderer_ ? renderer_->objects().army_pose(actor) : std::nullopt;
}

std::optional<scene::ArmyFigure> MapWindow::army_figure(std::uint32_t actor) const {
    if (!renderer_)
        return std::nullopt;
    return renderer_->army_figure(actor);
}

void MapWindow::float_text(core::Point cell, std::int32_t level, std::string text) {
    if (floating_font_ == nullptr || !renderer_)
        return;
    floating_.push_back({renderer_->army_pin(cell, level) - camera_, std::move(text), clock_ms_});
}

void MapWindow::set_clock(std::uint32_t clock_ms) {
    clock_ms_ = clock_ms;
    std::erase_if(floating_, [this](const Floating& f) { return clock_ms_ - f.born >= kFloatMs; });
}

void MapWindow::set_words(Words words) {
    set_help([this, words = std::move(words)](core::Point local) {
        return renderer_ ? words(hover_at(local)) : ui::Help{};
    });
}

std::optional<scene::MapHover> MapWindow::hover() const {
    if (!renderer_ || !pointer_)
        return std::nullopt;
    return hover_at(*pointer_);
}

scene::MapHover MapWindow::hover_at(core::Point local) const {
    return renderer_->hover_at(local + camera_, 0, clock_ms_);
}

cursor::CursorState MapWindow::pointer_here(core::Point local) const noexcept {
    const cursor::CursorState edge = cursor::map_edge_cursor(core::Rect{{}, rect().size()}, local);
    return edge.kind == cursor::CursorKind::move ? map_cursor_ : edge;
}

void MapWindow::on_pointer_moved(core::Point local) {
    pointer_ = local;
    if (renderer_)
        pointed.emit(hover_at(local));
}

void MapWindow::on_state_changed() {
    if (!hovered())
        pointer_.reset();
}

void MapWindow::on_click() {
    if (const std::optional<scene::MapHover> at = hover())
        clicked.emit(*at);
}

void MapWindow::draw(const graphics::Canvas& canvas) const {
    const graphics::Canvas window = canvas.child(core::Rect{{}, rect().size()});
    if (renderer_)
        renderer_->paint(window, camera_, 0, clock_ms_);
    if (floating_font_ == nullptr)
        return;
    const graphics::TextStyle style{graphics::colors::white, graphics::colors::black};
    for (const Floating& f : floating_) {
        const auto drift = static_cast<std::int32_t>((clock_ms_ - f.born) / kFloatStepMs);
        window.draw_text_line(*floating_font_, f.text, f.at - core::Point{drift, drift}, style);
    }
}

}
