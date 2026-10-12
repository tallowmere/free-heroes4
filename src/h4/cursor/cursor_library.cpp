#include "h4/cursor/cursor_library.hpp"

#include <algorithm>
#include <string>

#include "h4/assets/assets.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/rgba_image.hpp"

namespace h4::cursor {

namespace {

const formats::Layer* counted_face(const formats::BitmapGroup& group, std::int32_t days) {
    if (const auto* face = group.find(std::to_string(std::clamp(days, 1, 4))))
        return face;
    for (const int probe : {1, 2, 3, 4, 8})
        if (const auto* face = group.find(std::to_string(probe)))
            return face;
    return group.find("Layer 1");
}

core::Point hot_spot_of(const formats::Layer* guide, const formats::Layer& face) {
    return guide != nullptr ? guide->bounds().origin() : face.bounds().origin();
}

}

AnimationClock& CursorLibrary::clock(CursorKind kind) {
    AnimationClock& c = kind == CursorKind::arrow ? arrow_clock_ : cast_spell_clock_;
    if (!c.bound())
        if (const formats::Animation* a = assets_->animation(animation_name(kind)))
            c = AnimationClock{*a};
    return c;
}

CursorFrame CursorLibrary::animated_frame(CursorKind kind) {
    return CursorFrame{clock(kind).current(), {}};
}

CursorFrame CursorLibrary::static_frame(const CursorState& state) const {
    if (is_direction_kind(state.kind)) {
        const formats::BitmapGroup* g = assets_->group(direction_group_name(state.kind, state.dir16));
        const formats::Layer* face    = g != nullptr ? counted_face(*g, state.days) : nullptr;
        if (face == nullptr)
            return {};
        return {face, hot_spot_of(g->find("hot_spot"), *face)};
    }

    const std::string_view name = group_name(state.kind);
    if (name.empty())
        return {};
    const formats::BitmapGroup* g = assets_->group(name);
    if (g == nullptr)
        return {};

    const formats::Layer* face  = nullptr;
    const formats::Layer* guide = nullptr;
    switch (state.kind) {
    case CursorKind::scroll: {
        const std::string dir{to_string(state.dir8)};
        face  = g->find(dir);
        guide = g->find(dir + "_hot_spot");
        break;
    }
    case CursorKind::combat_melee:
    case CursorKind::combat_attack_gate:
        face = g->find(short_name(state.dir8));
        break;
    case CursorKind::combat_displacement:
        face = g->find(to_string(state.dir16));
        break;
    default:
        face  = counted_face(*g, state.days);
        guide = g->find("hot_spot");
        break;
    }
    if (face == nullptr)
        return {};
    return {face, hot_spot_of(guide, *face)};
}

CursorFrame CursorLibrary::frame(const CursorState& state) {
    return animation_name(state.kind).empty() ? static_frame(state) : animated_frame(state.kind);
}

bool CursorLibrary::has_art(CursorKind kind) const {
    if (const auto anim = animation_name(kind); !anim.empty())
        return assets_->animation(anim) != nullptr;
    if (is_direction_kind(kind))
        return assets_->group(direction_group_name(kind, Direction16::north)) != nullptr;
    return assets_->group(group_name(kind)) != nullptr;
}

const platform::CursorImage* CursorLibrary::image(const CursorState& state) {
    const CursorFrame f = frame(state);
    if (f.art == nullptr)
        return nullptr;

    const ImageKey key{f.art, f.hot_spot.x, f.hot_spot.y};
    if (const auto it = images_.find(key); it != images_.end())
        return &it->second;

    const core::Rect box       = f.art->bounds().united(core::Rect{f.hot_spot, {1, 1}});
    graphics::RgbaImage pixels = graphics::rasterize_layer(*f.art, box);
    auto made                  = platform::CursorImage::make(
        platform::CursorImageId{next_image_id_++}, pixels.size, f.hot_spot - box.origin(), std::move(pixels.pixels)
    );
    if (!made)
        return nullptr;
    return &images_.emplace(key, std::move(*made)).first->second;
}

void CursorLibrary::advance(core::Instant now) {
    clock(CursorKind::arrow).advance(now);
    clock(CursorKind::combat_cast_spell).advance(now);
}

}
