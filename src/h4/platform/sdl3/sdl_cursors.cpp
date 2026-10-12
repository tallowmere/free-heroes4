#include "h4/platform/sdl3/sdl_cursors.hpp"

#include <algorithm>
#include <utility>

#include "h4/graphics/nearest.hpp"
#include "h4/graphics/rgba_image.hpp"

namespace h4::platform::sdl3 {

namespace {

graphics::RgbaImage scale_nearest(const CursorImage& image, float scale) {
    return graphics::scale_nearest(graphics::RgbaImage{image.size(), {image.rgba().begin(), image.rgba().end()}},
                                   static_cast<double>(scale));
}

int scaled_hot(std::int32_t hot, float scale, std::int32_t length) {
    return std::clamp(graphics::first_destination(hot, static_cast<double>(scale)), 0, length - 1);
}

}

PointerCursors::~PointerCursors() {
    if (current_)
        SDL_SetCursor(SDL_GetDefaultCursor());
}

void PointerCursors::show(const CursorImage* image) {
    if (image == nullptr) {
        current_.reset();
        SDL_SetCursor(SDL_GetDefaultCursor());
        return;
    }
    if (current_ && current_->id() == image->id())
        return;
    current_ = *image;
    show_current();
}

void PointerCursors::rescale() {
    auto stale = std::move(cursors_);
    cursors_.clear();
    if (current_)
        show_current();
}

void PointerCursors::set_logical(core::Size logical) {
    logical_ = logical;
    rescale();
}

void PointerCursors::show_current() {
    auto it = cursors_.find(current_->id().value());
    if (it == cursors_.end())
        it = cursors_.emplace(current_->id().value(), build(*current_)).first;
    SDL_SetCursor(it->second ? it->second.get() : SDL_GetDefaultCursor());
}

float PointerCursors::point_scale() const {
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);
    const float sx = static_cast<float>(w) / static_cast<float>(logical_.width);
    const float sy = static_cast<float>(h) / static_cast<float>(logical_.height);
    return std::max(0.25F, std::min(sx, sy));
}

SdlHandle<SDL_Cursor> PointerCursors::build(const CursorImage& image) const {
    if (image.size().empty())
        return nullptr;
    const float scale   = point_scale();
    const float density = std::max(1.0F, SDL_GetWindowPixelDensity(window_));

    graphics::RgbaImage base = scale_nearest(image, scale);
    SdlHandle<SDL_Surface> surface{SDL_CreateSurfaceFrom(
        base.size.width, base.size.height, SDL_PIXELFORMAT_RGBA32, base.pixels.data(), base.size.width * 4
    )};
    if (!surface)
        return nullptr;

    graphics::RgbaImage dense;
    if (density > 1.0F) {
        dense = scale_nearest(image, scale * density);
        SdlHandle<SDL_Surface> alternate{SDL_CreateSurfaceFrom(
            dense.size.width, dense.size.height, SDL_PIXELFORMAT_RGBA32, dense.pixels.data(), dense.size.width * 4
        )};
        if (alternate)
            SDL_AddSurfaceAlternateImage(surface.get(), alternate.get());
    }

    const int hot_x = scaled_hot(image.hot_spot().x, scale, base.size.width);
    const int hot_y = scaled_hot(image.hot_spot().y, scale, base.size.height);
    return SdlHandle<SDL_Cursor>{SDL_CreateColorCursor(surface.get(), hot_x, hot_y)};
}

}
