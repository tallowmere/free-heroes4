#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/rules/adventure_path.hpp"
#include "h4/scene/map_objects.hpp"

namespace h4::app {

class ArmyMover {
public:
    struct Arrival {
        rules::MapPoint at;
        std::int32_t total = 0;
    };
    using Arrive = std::function<bool(const Arrival& arrival)>;
    using TileHook = std::function<bool()>;

    ArmyMover(const rules::MapPoint& start,
              int facing,
              std::vector<rules::PathField::PathStep> steps,
              scene::ArmyFigure figure,
              std::uint32_t now,
              std::int32_t speed = 150);

    void tick(std::uint32_t now, const Arrive& arrive, const TileHook& hook = {});

    void stop() {
        if (!finished_)
            finish();
    }

    void delay(std::uint32_t ms) noexcept {
        due_ += ms;
    }

    [[nodiscard]] bool finished() const noexcept {
        return finished_;
    }
    [[nodiscard]] rules::MapPoint point() const noexcept {
        return point_;
    }
    [[nodiscard]] scene::ArmyPose pose() const noexcept;

private:
    void recompute(scene::ArmyGroup group);
    void begin_step();
    void plan_phase(std::uint32_t frame);
    struct Phase {
        std::int32_t substeps = 0;
        std::int32_t distance = 0;
    };
    [[nodiscard]] Phase fit_phase(std::uint32_t frame);
    void scale_phase(std::int32_t substeps, std::int32_t distance);
    void set_cadence();
    void arrive(const Arrive& arrive);
    void finish();
    [[nodiscard]] core::Point pin_of(const rules::MapPoint& at) const;

    scene::ArmyFigure figure_;
    std::vector<rules::PathField::PathStep> steps_;
    std::int32_t index_ = -1;
    rules::MapPoint point_;
    std::int32_t speed_;

    scene::ArmyGroup group_ = scene::ArmyGroup::wait;
    int facing_;
    std::uint32_t frame_    = 0;
    std::uint32_t frames_   = 0;
    std::uint32_t frame_ms_ = 0;
    core::Point offset_;

    std::int32_t phase_left_           = 0;
    std::int32_t arrive_at_            = 0;
    std::int32_t delta_x_              = 0;
    std::int32_t delta_y_              = 0;
    std::int32_t distance_left_        = 0;
    std::int32_t distance_per_substep_ = 0;
    std::int32_t scale_                = 1;
    std::int32_t substeps_left_        = 0;
    std::int32_t pixel_x_              = 0;
    std::int32_t pixel_y_              = 0;
    std::uint32_t tick_ms_             = 0;
    std::uint32_t due_                 = 0;
    bool finished_                     = false;
};

}
