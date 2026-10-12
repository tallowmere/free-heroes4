#include "h4/ui/widgets/dialog_frame.hpp"

#include "h4/graphics/canvas.hpp"

namespace h4::ui {

namespace {

void tile_run(const graphics::Canvas& canvas, const formats::Layer& tile, const core::Rect& run) {
    const core::Size step = tile.bounds().size();
    if (run.empty() || step.empty())
        return;
    const graphics::Canvas clipped = canvas.child(run);
    for (std::int32_t y = 0; y < run.height(); y += step.height)
        for (std::int32_t x = 0; x < run.width(); x += step.width)
            clipped.draw_layer(tile, core::Point{x, y} - tile.bounds().origin());
}

core::Rect moved(const formats::Layer& layer, std::int32_t dx, std::int32_t dy) {
    return layer.bounds().translated(core::Point{dx, dy});
}

}

std::optional<DialogFrameArt> DialogFrameArt::from_group(const formats::BitmapGroup& group) {
    DialogFrameArt art{group.find("top_left"),
                       group.find("top"),
                       group.find("top_right"),
                       group.find("left"),
                       group.find("right"),
                       group.find("bottom_left"),
                       group.find("bottom"),
                       group.find("bottom_right"),
                       group.find("background"),
                       {},
                       {}};
    const formats::Layer* fill   = group.find("background_rect");
    const formats::Layer* client = group.find("client_area");
    for (const formats::Layer* piece :
         {art.top_left, art.top, art.top_right, art.left, art.right, art.bottom_left, art.bottom, art.bottom_right, art.background, fill, client})
        if (piece == nullptr)
            return std::nullopt;
    art.background_rect = fill->bounds();
    art.client_area     = client->bounds();
    return art;
}

core::Rect DialogFrameArt::bounds() const noexcept {
    core::Rect out = top_left->bounds();
    for (const formats::Layer* piece : {top, top_right, left, right, bottom_left, bottom, bottom_right})
        out = out.united(piece->bounds());
    return out;
}

FrameInsets DialogFrameArt::insets() const noexcept {
    const core::Rect outer = bounds();
    return {client_area.left() - outer.left(),
            client_area.top() - outer.top(),
            outer.right() - client_area.right(),
            outer.bottom() - client_area.bottom()};
}

DialogFrame::DialogFrame(core::Rect rect, const DialogFrameArt& art) noexcept
: Widget{rect}, art_{art}, growth_{rect.width() - art.bounds().width(), rect.height() - art.bounds().height()} {
}

core::Rect DialogFrame::client_rect() const noexcept {
    const core::Rect c = art_.client_area.translated(core::Point{} - art_.bounds().origin());
    return core::Rect{c.origin(), {c.width() + growth_.x, c.height() + growth_.y}};
}

void DialogFrame::draw(const graphics::Canvas& canvas) const {
    const graphics::Canvas local = canvas.translated(core::Point{} - art_.bounds().origin());
    const std::int32_t gx        = growth_.x;
    const std::int32_t gy        = growth_.y;
    const core::Rect tl          = art_.top_left->bounds();
    const core::Rect tr          = art_.top_right->bounds();
    const core::Rect bl          = art_.bottom_left->bounds();
    const core::Rect br          = art_.bottom_right->bounds();

    const core::Rect fill = art_.background_rect;
    tile_run(local, *art_.background, core::Rect{fill.origin(), {fill.width() + gx, fill.height() + gy}});
    tile_run(local,
             *art_.left,
             core::Rect::from_edges(art_.left->bounds().left(), tl.bottom(), art_.left->bounds().right(), bl.top() + gy));
    tile_run(local,
             *art_.right,
             core::Rect::from_edges(art_.right->bounds().left() + gx, tr.bottom(), art_.right->bounds().right() + gx, br.top() + gy));
    tile_run(local,
             *art_.top,
             core::Rect::from_edges(tl.right(), art_.top->bounds().top(), tr.left() + gx, art_.top->bounds().bottom()));
    tile_run(local,
             *art_.bottom,
             core::Rect::from_edges(bl.right(), art_.bottom->bounds().top() + gy, br.left() + gx, art_.bottom->bounds().bottom() + gy));

    local.draw_layer(*art_.top_left);
    local.draw_layer(*art_.top_right, moved(*art_.top_right, gx, 0).origin() - tr.origin());
    local.draw_layer(*art_.bottom_left, moved(*art_.bottom_left, 0, gy).origin() - bl.origin());
    local.draw_layer(*art_.bottom_right, core::Point{gx, gy});
}

}
