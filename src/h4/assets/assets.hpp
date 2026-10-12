#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "h4/assets/prefetched.hpp"
#include "h4/assets/request.hpp"
#include "h4/core/pending.hpp"

namespace h4::formats {
class ActorSequence;
class AdvActor;
class CombatActor;
class AdvObject;
class Animation;
class BitmapGroup;
class Font;
class TerrainTexture;
class TransitionLibrary;
}

namespace h4::graphics {
class Surface565;
}

namespace h4::assets {

struct InterfaceHelp {
    std::string_view balloon;
    std::string_view right_click;
};

class Assets {
public:
    virtual ~Assets() = default;

    [[nodiscard]] virtual const formats::BitmapGroup* group(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::Animation* animation(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::TerrainTexture* terrain(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::AdvObject* object(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::AdvActor* actor(std::string_view name) const         = 0;
    [[nodiscard]] virtual const formats::ActorSequence* sequence(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::CombatActor* combat_actor(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::TransitionLibrary* transitions() const = 0;

    [[nodiscard]] virtual const graphics::Surface565* painting(std::string_view name) const = 0;

    [[nodiscard]] virtual const formats::Font* font_for_size(std::int32_t asked) const = 0;

    [[nodiscard]] virtual std::string_view text(std::string_view key) const = 0;

    [[nodiscard]] virtual InterfaceHelp help(std::string_view window, std::string_view item) const = 0;

    [[nodiscard]] virtual std::vector<std::byte> game_map(std::string_view name) const = 0;

    [[nodiscard]] virtual core::Pending<Prefetched> prefetch(std::span<const AssetRequest> requests) const = 0;
    virtual void adopt(Prefetched batch) const = 0;

protected:
    Assets()                         = default;
    Assets(const Assets&)            = default;
    Assets& operator=(const Assets&) = default;
    Assets(Assets&&)                 = default;
    Assets& operator=(Assets&&)      = default;
};

}
