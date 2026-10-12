#pragma once

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "h4/assets/assets.hpp"
#include "h4/core/ascii.hpp"
#include "h4/formats/actor_sequence.hpp"
#include "h4/formats/adv_actor.hpp"
#include "h4/formats/adv_object.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/combat_actor.hpp"
#include "h4/formats/font.hpp"
#include "h4/formats/terrain_texture.hpp"
#include "h4/formats/transition_set.hpp"
#include "h4/graphics/surface565.hpp"

namespace h4::io {
class ResourceSource;
}

namespace h4::assets {

class AssetLibrary final : public Assets {
public:
    explicit AssetLibrary(const io::ResourceSource& source);

    AssetLibrary(const AssetLibrary&)            = delete;
    AssetLibrary& operator=(const AssetLibrary&) = delete;

    [[nodiscard]] const formats::BitmapGroup* group(std::string_view name) const override;
    [[nodiscard]] const formats::TerrainTexture* terrain(std::string_view name) const override;
    [[nodiscard]] const formats::Animation* animation(std::string_view name) const override;
    [[nodiscard]] const formats::AdvObject* object(std::string_view name) const override;
    [[nodiscard]] const formats::AdvActor* actor(std::string_view name) const override;
    [[nodiscard]] const formats::ActorSequence* sequence(std::string_view name) const override;
    [[nodiscard]] const formats::CombatActor* combat_actor(std::string_view name) const override;
    [[nodiscard]] const formats::TransitionLibrary* transitions() const override;
    [[nodiscard]] const graphics::Surface565* painting(std::string_view name) const override;
    [[nodiscard]] const formats::Font* font_for_size(std::int32_t asked) const override;
    [[nodiscard]] std::string_view text(std::string_view key) const override;
    [[nodiscard]] InterfaceHelp help(std::string_view window, std::string_view item) const override;

    [[nodiscard]] std::vector<std::byte> game_map(std::string_view name) const override;

    [[nodiscard]] core::Pending<Prefetched> prefetch(std::span<const AssetRequest> requests) const override;
    void adopt(Prefetched done) const override;

private:
    [[nodiscard]] const formats::Font* font(std::string_view entry) const;

    template <class T> using Cache = core::FoldedMap<std::unique_ptr<const T>>;

    const io::ResourceSource* source_;
    mutable Cache<formats::BitmapGroup> groups_;
    mutable Cache<formats::Animation> animations_;
    mutable Cache<formats::TerrainTexture> terrains_;
    mutable Cache<formats::AdvObject> objects_;
    mutable Cache<formats::AdvActor> actors_;
    mutable Cache<formats::ActorSequence> sequences_;
    mutable Cache<formats::CombatActor> combat_actors_;
    mutable std::unique_ptr<const formats::TransitionLibrary> transitions_;
    mutable bool transitions_read_ = false;
    mutable Cache<graphics::Surface565> paintings_;
    mutable Cache<formats::Font> fonts_;
    core::FoldedMap<std::string> strings_;
    core::FoldedMap<std::array<std::string, 2>> helps_;
};

}
