#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <tuple>

#include "h4/core/geometry.hpp"
#include "h4/cursor/animation_clock.hpp"
#include "h4/cursor/cursor_source.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/platform/cursor_image.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
}

namespace h4::cursor {

struct CursorFrame {
    const formats::Layer* art = nullptr;
    core::Point hot_spot;
};

class CursorLibrary final : public CursorSource {
public:
    explicit CursorLibrary(const assets::Assets& assets) noexcept : assets_{&assets} {
    }

    CursorLibrary(const CursorLibrary&)            = delete;
    CursorLibrary& operator=(const CursorLibrary&) = delete;

    [[nodiscard]] CursorFrame frame(const CursorState& state);

    [[nodiscard]] bool has_art(CursorKind kind) const;

    [[nodiscard]] const platform::CursorImage* image(const CursorState& state) override;
    void advance(core::Instant now) override;

private:
    using ImageKey = std::tuple<const formats::Layer*, std::int32_t, std::int32_t>;

    [[nodiscard]] AnimationClock& clock(CursorKind animated_kind);
    [[nodiscard]] CursorFrame animated_frame(CursorKind kind);
    [[nodiscard]] CursorFrame static_frame(const CursorState& state) const;

    const assets::Assets* assets_;
    AnimationClock arrow_clock_;
    AnimationClock cast_spell_clock_;
    std::map<ImageKey, platform::CursorImage> images_;
    std::uint32_t next_image_id_ = 1;
};

}
