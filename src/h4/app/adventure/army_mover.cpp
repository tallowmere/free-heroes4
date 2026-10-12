#include "h4/app/adventure/army_mover.hpp"

#include <algorithm>
#include <utility>

namespace h4::app {

namespace {

using scene::ArmyGroup;

[[nodiscard]] std::int32_t step_distance(int dir) noexcept {
    return (dir & 1) != 0 ? 0x30 : 0x20;
}

}

ArmyMover::ArmyMover(const rules::MapPoint& start,
                     int facing,
                     std::vector<rules::PathField::PathStep> steps,
                     scene::ArmyFigure figure,
                     std::uint32_t now,
                     std::int32_t speed)
: figure_{std::move(figure)}, steps_{std::move(steps)}, point_{start}, speed_{speed > 0 ? speed : 150}, facing_{facing & 7} {
    due_ = now;
    if (steps_.empty()) {
        finished_ = true;
        return;
    }
    for (const rules::PathField::PathStep& step : steps_)
        distance_left_ += step_distance(step.dir);
    recompute(ArmyGroup::prewalk);
    begin_step();
    plan_phase(0);
    due_ = now;
}

scene::ArmyPose ArmyMover::pose() const noexcept {
    scene::ArmyPose out{group_, facing_, std::nullopt, offset_, point_.bridge};
    if (!finished_)
        out.frame = frame_;
    return out;
}

core::Point ArmyMover::pin_of(const rules::MapPoint& at) const {
    return figure_.pin({at.x, at.y}, at.level, at.bridge);
}

void ArmyMover::recompute(ArmyGroup group) {
    group_    = group;
    frame_    = 0;
    frames_   = std::max<std::uint32_t>(figure_.frames ? figure_.frames(group, facing_) : 1U, 1U);
    frame_ms_ = figure_.frame_ms ? figure_.frame_ms(group) : 0U;
    if (frame_ms_ == 0)
        frame_ms_ = 100;
}

void ArmyMover::begin_step() {
    if (distance_left_ == 0)
        return;
    ++index_;
    const rules::PathField::PathStep& node = steps_[static_cast<std::size_t>(index_)];
    phase_left_                            = step_distance(node.dir);
    distance_left_ -= phase_left_;
    arrive_at_ = phase_left_ >> 1;
    facing_    = node.dir & 7;
}

void ArmyMover::plan_phase(std::uint32_t frame) {
    const Phase phase = fit_phase(frame);
    scale_phase(std::max(phase.substeps, 1), phase.distance);
    set_cadence();
}

ArmyMover::Phase ArmyMover::fit_phase(std::uint32_t frame) {
    const std::int32_t total  = distance_left_ + phase_left_;
    const std::int32_t d_pre  = figure_.walk.prewalk;
    const std::int32_t d_walk = std::max(figure_.walk.walk, 1);
    const std::int32_t d_post = figure_.walk.postwalk;
    const auto f              = static_cast<std::int32_t>(frame);
    const auto frames         = static_cast<std::int32_t>(frames_);

    std::int32_t substeps = 0;
    std::int32_t distance = 0;
    if (group_ == ArmyGroup::postwalk) {
        substeps = std::max(frames - f, 1);
        distance = phase_left_;
    } else {
        std::int32_t pre_part = 0;
        if (group_ == ArmyGroup::prewalk && f < frames)
            pre_part = ((frames - f) * d_pre) / frames;
        const bool walking = group_ == ArmyGroup::walk;
        std::int32_t rest  = total - d_post - pre_part;
        if (walking)
            rest += (d_walk * f) / frames;
        std::int32_t cycles = std::max(rest / d_walk, 0);
        if ((pre_part + d_post) * 2 < rest - cycles * d_walk)
            ++cycles;
        if (walking && f != 0 && cycles == 0)
            cycles = 1;
        std::int32_t covered = cycles * d_walk;
        std::int32_t span    = covered + d_post + pre_part;
        if (walking && f != 0)
            span -= (d_walk * f) / frames;
        const auto share = [&](std::int32_t part) { return span > 0 ? (part * total) / span : 0; };

        if (walking && cycles > 0) {
            covered -= (d_walk * f) / frames;
            distance = share(covered);
            substeps = cycles * frames - f;
        } else if (group_ == ArmyGroup::prewalk && f < frames) {
            distance = share(pre_part);
            substeps = frames - f;
        } else if (cycles > 0) {
            recompute(ArmyGroup::walk);
            distance = share(cycles * d_walk);
            substeps = cycles * static_cast<std::int32_t>(frames_);
        } else {
            recompute(ArmyGroup::postwalk);
            substeps = std::max(static_cast<std::int32_t>(frames_), 1);
            distance = phase_left_;
        }
    }
    return {substeps, distance};
}

void ArmyMover::scale_phase(std::int32_t substeps, std::int32_t distance) {
    const rules::MapPoint& to_point = steps_[static_cast<std::size_t>(std::max(index_, 0))].at;
    const core::Point from          = pin_of(point_) + offset_;
    const core::Point to            = pin_of(to_point);
    const std::int32_t dx           = to.x - from.x;
    const std::int32_t dy           = to.y - from.y;
    delta_x_                        = dx;
    delta_y_                        = dy;
    distance_per_substep_           = phase_left_;
    distance                        = std::max(distance, 0);
    if (distance < phase_left_) {
        if (distance > 0) {
            distance_per_substep_ = distance;
            delta_x_              = dx * distance / phase_left_;
            delta_y_              = dy * distance / phase_left_;
        }
    } else if (distance > phase_left_ && phase_left_ > 0)
        substeps = std::max((phase_left_ * substeps + distance - 1) / distance, 1);

    scale_         = substeps;
    substeps_left_ = substeps;
    phase_left_ *= substeps;
    arrive_at_ *= substeps;
    pixel_x_ = offset_.x * scale_;
    pixel_y_ = offset_.y * scale_;
}

void ArmyMover::set_cadence() {
    const auto tick = static_cast<std::uint32_t>(frame_ms_ * 100U / static_cast<std::uint32_t>(speed_));
    due_            = due_ - tick_ms_ + tick;
    tick_ms_        = tick;
}

void ArmyMover::arrive(const Arrive& arrive) {
    arrive_at_                             = -scale_;
    const rules::PathField::PathStep& node = steps_[static_cast<std::size_t>(index_)];
    const core::Point from                 = pin_of(point_);
    const core::Point to                   = pin_of(node.at);
    pixel_x_ -= (to.x - from.x) * scale_;
    pixel_y_ -= (to.y - from.y) * scale_;
    if (arrive && !arrive(Arrival{node.at, node.total})) {
        finish();
        return;
    }
    point_ = node.at;
}

void ArmyMover::finish() {
    finished_ = true;
    recompute(ArmyGroup::wait);
    offset_ = {};
}

void ArmyMover::tick(std::uint32_t now, const Arrive& arrive, const TileHook& hook) {
    std::uint32_t frame = frame_;
    while (!finished_ && substeps_left_ > 0 && static_cast<std::int32_t>(now - due_) >= 0) {
        pixel_x_ += delta_x_;
        pixel_y_ += delta_y_;
        phase_left_ -= distance_per_substep_;
        if (phase_left_ <= arrive_at_) {
            this->arrive(arrive);
            if (finished_)
                return;
        }
        offset_ = {pixel_x_ / scale_, pixel_y_ / scale_};
        ++frame;
        if (group_ == ArmyGroup::walk && frame == frames_)
            frame = 0;
        if (frame < frames_)
            frame_ = frame;
        --substeps_left_;
        due_ += tick_ms_;
        if (substeps_left_ > 0)
            continue;

        if ((group_ == ArmyGroup::postwalk || steps_.empty()) && distance_left_ + phase_left_ == 0) {
            finish();
            return;
        }
        if (phase_left_ == 0) {
            begin_step();
            if (hook && !hook()) {
                finish();
                return;
            }
        } else {
            phase_left_ /= scale_;
            arrive_at_ /= scale_;
        }
        plan_phase(frame);
        frame = frame_;
    }
}

}
