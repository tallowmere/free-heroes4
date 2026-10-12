#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/scene/map_path.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Animation;
class AdvActor;
class AdvObject;
class BitmapGroup;
class Layer;
struct SubImageLayers;
}
namespace h4::graphics {
class Canvas;
}
namespace h4::model {
class Map;
struct MapActor;
class Shroud;
}

namespace h4::scene {

enum class ArmyGroup : std::uint8_t { wait, prewalk, walk, postwalk };
struct ArmyPose {
    ArmyGroup group = ArmyGroup::wait;
    int facing      = 0;
    std::optional<std::uint32_t> frame;
    core::Point offset;
    bool bridge = false;
};

struct ArmyWalk {
    std::int32_t prewalk  = 0x10;
    std::int32_t walk     = 0x20;
    std::int32_t postwalk = 0x10;
};

struct ArmyFigure {
    ArmyWalk walk;
    std::function<std::uint32_t(ArmyGroup group, int facing)> frames;
    std::function<std::uint32_t(ArmyGroup group)> frame_ms;
    std::function<core::Point(core::Point cell, std::int32_t level, bool bridge)> pin;
};

class MapObjects {
public:
    MapObjects(const assets::Assets& assets, const model::Map& map);

    void select(const model::MapActor* army);
    [[nodiscard]] std::optional<std::uint32_t> selected() const noexcept {
        return selected_;
    }

    void pose_army(std::uint32_t actor, core::Point cell, std::int32_t level, const ArmyPose& pose);
    [[nodiscard]] std::int32_t army_height(core::Point cell, std::int32_t level, bool bridge) const;
    void remove(std::uint32_t actor) noexcept;
    [[nodiscard]] std::optional<std::pair<core::Point, std::int32_t>> army_cell(std::uint32_t actor) const noexcept;
    [[nodiscard]] std::optional<ArmyPose> army_pose(std::uint32_t actor) const noexcept;
    [[nodiscard]] std::uint32_t army_frames(std::uint32_t actor, ArmyGroup group, int facing) const;
    [[nodiscard]] std::uint32_t army_frame_ms(std::uint32_t actor, ArmyGroup group) const noexcept;
    [[nodiscard]] ArmyWalk army_walk(std::uint32_t actor) const noexcept;

    [[nodiscard]] std::optional<core::Point> pin_of(const model::MapActor& actor) const noexcept;
    [[nodiscard]] std::optional<core::Point> path_pin(std::size_t index) const noexcept;
    [[nodiscard]] bool drawn(const model::MapActor& actor, const model::Shroud* seen) const noexcept;
    [[nodiscard]] std::optional<core::Point> marker_pin() const noexcept;

    void paint(const graphics::Canvas& canvas,
               core::Point camera,
               std::int32_t level,
               const model::Shroud* seen = nullptr,
               std::uint32_t now         = 0) const;

    void show_path(std::span<const PathMark> marks);

    [[nodiscard]] std::optional<std::uint32_t>
    pick(core::Point point, std::int32_t level, const model::Shroud* seen, std::uint32_t now) const;

private:
    struct Overlay {
        const formats::Animation* animation = nullptr;
        core::Point at;
        std::uint32_t phase = 0;
    };

    struct Placed {
        const formats::AdvObject* model       = nullptr;
        const formats::BitmapGroup* group     = nullptr;
        const formats::SubImageLayers* layers = nullptr;
        core::Point art;
        core::Point cell;
        core::Size box;
        core::Point pin;
        core::Rect rect;
        std::int32_t depth  = 0;
        std::int32_t level  = 0;
        std::uint32_t order = 0;
        Overlay ring;
        Overlay flag;
        std::uint32_t frame_ms = 0;
        std::uint32_t phase    = 0;
        bool underlay = false;
        bool gone = false;
        std::int32_t army_span = 0;
    };

    void place_army(std::uint32_t actor, int facing);
    [[nodiscard]] std::optional<Placed>
    army_placement(std::uint32_t actor, core::Point cell, std::int32_t level, const ArmyPose& pose) const;

    static void grow_to_overlays(Placed& placed);

    [[nodiscard]] static Overlay
    step_of(const formats::Animation& animation, core::Point pin, core::Point position, std::uint32_t phase);

    enum class Standing : std::uint8_t { footprint, corners, deck };
    [[nodiscard]] Placed place_model(const formats::AdvObject& model,
                                     core::Point cell,
                                     std::int32_t level,
                                     std::uint32_t order,
                                     Standing standing = Standing::footprint) const;
    [[nodiscard]] std::int32_t deck_height(core::Point cell, core::Size box, std::int32_t level) const;

    [[nodiscard]] static bool in_sight(const Placed& placed, std::int32_t level, const model::Shroud& seen);
    [[nodiscard]] static bool shown(const Placed& placed, std::int32_t level, const model::Shroud* seen);
    [[nodiscard]] static bool hit(const Placed& placed, core::Point point, std::uint32_t now);
    [[nodiscard]] static const formats::Layer* overlay_layer(const Overlay& piece, std::uint32_t now) noexcept;
    static void sort_column(std::span<const Placed* const> list,
                            const core::Rect& band,
                            std::vector<std::pair<std::int32_t, const Placed*>>& order);

    void draw_frames(const graphics::Canvas& canvas,
                     core::Point camera,
                     const core::Rect& clip,
                     std::span<const Placed* const> list,
                     std::uint32_t now) const;

    const assets::Assets* assets_;
    const model::Map* map_;
    std::vector<Placed> placed_;
    struct Figure {
        const formats::AdvActor* actor = nullptr;
        std::int32_t placed            = -1;
        core::Point cell;
        std::int32_t level = 0;
        ArmyPose pose;
    };
    std::vector<Figure> figures_;
    std::unordered_map<std::int64_t, std::int32_t> decks_;
    std::optional<std::uint32_t> selected_;
    std::optional<Placed> marker_;
    std::vector<Placed> path_;
    mutable std::vector<const Placed*> under_;
    mutable std::vector<const Placed*> over_;
    mutable std::vector<std::pair<std::int32_t, const Placed*>> column_order_;
};

}
