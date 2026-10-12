#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/model/alignment.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
}
namespace h4::scene {
class MiniMap;
}
namespace h4::ui {
class ClickArea;
class Painting;
class Sprite;
}

namespace h4::screens {

class Builder;

struct TownBar {
    int built = 0;
    int total = 0;
};

struct TownListEntry {
    std::optional<model::Alignment> alignment;
    std::string_view ground        = "grass";
    std::string_view fortification = "village";
    std::array<TownBar, 3> bars{};
    core::Rect tiles;
};

class TownListView final : public ui::Widget {
public:
    TownListView(Builder& builder, core::Rect guide);

    void set_map(const scene::MiniMap* thumbnail, std::int32_t dimension) noexcept {
        thumbnail_ = thumbnail;
        dimension_ = dimension;
    }
    void set_entries(std::span<const TownListEntry> entries);
    void select(std::optional<std::size_t> row);

    [[nodiscard]] core::Size thumbnail_size() const noexcept {
        return thumbnail_size_;
    }
    [[nodiscard]] std::size_t rows() const noexcept {
        return rows_.size();
    }
    [[nodiscard]] core::Rect row_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect marker_rect(std::size_t row) const noexcept {
        return rows_[row].marker;
    }
    [[nodiscard]] const scene::MiniMap* map() const noexcept {
        return thumbnail_;
    }
    [[nodiscard]] const ui::Widget& thumbnail(std::size_t row) const noexcept {
        return *rows_[row].map;
    }
    [[nodiscard]] const graphics::Surface565& picture(std::size_t row) const noexcept {
        return *rows_[row].picture;
    }

    core::Signal<std::size_t> row_pressed;

private:
    struct Row {
        ui::ClickArea* picture_window = nullptr;
        ui::Widget* map               = nullptr;
        ui::Painting* map_image       = nullptr;
        ui::Sprite* pin               = nullptr;
        ui::Widget* frame             = nullptr;
        ui::Widget* highlight         = nullptr;
        ui::Widget* built             = nullptr;
        std::unique_ptr<graphics::Surface565> picture;
        core::Rect marker;
    };

    const assets::Assets* assets_;
    std::vector<Row> rows_;
    core::Size thumbnail_size_;
    const scene::MiniMap* thumbnail_ = nullptr;
    std::int32_t dimension_          = 0;
    const formats::Layer* marker_    = nullptr;
    core::Connections connections_;
};

}
