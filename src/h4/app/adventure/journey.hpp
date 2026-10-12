#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "h4/app/adventure/army_mover.hpp"
#include "h4/rules/adventure_path.hpp"
#include "h4/scene/map_objects.hpp"

namespace h4::app {

class Journey {
public:
    struct Visit {
        std::uint32_t object = 0;
        rules::PathField::PathStep step;
        rules::MapPoint from;
    };

    void begin(std::uint32_t actor, std::optional<Visit> visit, std::unique_ptr<ArmyMover> mover);

    [[nodiscard]] bool walking() const noexcept {
        return mover_ != nullptr;
    }
    [[nodiscard]] std::uint32_t actor() const noexcept {
        return actor_;
    }
    [[nodiscard]] const ArmyMover& mover() const noexcept {
        return *mover_;
    }

    void charged_to(std::int32_t total) noexcept {
        charged_ = total;
    }
    [[nodiscard]] std::int32_t uncharged(std::int32_t total) const noexcept {
        return total - charged_;
    }

    void tick(std::uint32_t now, bool held, const ArmyMover::Arrive& arrive, const ArmyMover::TileHook& hook);
    void stop() {
        mover_->stop();
    }

    struct Rest {
        rules::MapPoint at;
        scene::ArmyPose pose;
    };
    [[nodiscard]] std::optional<Rest> end();
    [[nodiscard]] std::optional<Visit> visit_from(const rules::MapPoint& at);

private:
    std::uint32_t actor_  = 0;
    std::int32_t charged_ = 0;
    std::optional<Visit> visit_;
    std::unique_ptr<ArmyMover> mover_;
    std::optional<std::uint32_t> held_since_;
};

}
