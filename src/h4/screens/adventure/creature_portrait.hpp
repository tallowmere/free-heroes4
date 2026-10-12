#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "h4/model/roller.hpp"
#include "h4/screens/adventure/creature_idle.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class ActorSequence;
class BitmapGroup;
class CombatActor;
class Layer;
}

namespace h4::screens {

struct CreatureBoxModel {
    std::string_view model;
    std::string_view alignment;
};

class CreaturePortrait final : public ui::Widget {
public:
    CreaturePortrait(core::Rect rect, const assets::Assets& assets, CreatureBoxModel model);

    void advance(core::Instant now);

    [[nodiscard]] std::size_t copies() const noexcept {
        return copies_;
    }
    [[nodiscard]] double scale() const noexcept {
        return scale_;
    }
    [[nodiscard]] formats::CombatState state(std::size_t i) const noexcept {
        return copies_idle_[i]->state();
    }
    [[nodiscard]] std::size_t frame(std::size_t i) const noexcept {
        return copies_idle_[i]->frame();
    }

protected:
    void draw(const graphics::Canvas& canvas) const override;

private:
    [[nodiscard]] CreatureIdle::Model load_states(const assets::Assets& assets, const formats::CombatActor& actor);
    [[nodiscard]] static const formats::ActorSequence* shown_sequence(const assets::Assets& assets, const formats::CombatActor& actor);
    void place_copies(core::Rect extent, const CreatureIdle::Model& idle);

    const formats::BitmapGroup* background_ = nullptr;
    const formats::BitmapGroup* layout_     = nullptr;
    const formats::ActorSequence* sequence_ = nullptr;
    double scale_                           = 1.0;
    std::size_t copies_                     = 0;
    std::array<core::Point, 3> anchors_{};
    std::array<const formats::ActorSequence*, CreatureIdle::kStates> states_{};
    std::array<std::optional<CreatureIdle>, 3> copies_idle_;
    model::Roller seed_{1};
};

}
