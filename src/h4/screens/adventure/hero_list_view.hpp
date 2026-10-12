#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/ui/help.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
}
namespace h4::ui {
class ClickArea;
class IconCell;
class Meter;
}

namespace h4::screens {

class Builder;

struct HeroListEntry {
    const formats::Layer* face = nullptr;
    ui::Help help;
    int move     = 0;
    int strength = 0;
    ui::Help move_help;
};

class HeroListView final : public ui::Widget {
public:
    HeroListView(Builder& builder, core::Rect guide);

    void set_entries(std::span<const HeroListEntry> entries);
    void hide_rows_from(std::size_t first);
    void select(std::optional<std::size_t> row);

    [[nodiscard]] std::size_t rows() const noexcept {
        return rows_.size();
    }
    [[nodiscard]] core::Rect row_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect move_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect strength_rect(std::size_t row) const noexcept;

    core::Signal<std::size_t> row_pressed;
    core::Signal<std::size_t> row_double_clicked;

private:
    struct Row {
        ui::Widget* empty       = nullptr;
        ui::Meter* move         = nullptr;
        ui::Meter* strength     = nullptr;
        ui::ClickArea* portrait = nullptr;
        ui::IconCell* face      = nullptr;
        ui::Widget* frame       = nullptr;
        ui::Widget* highlight   = nullptr;
    };

    const assets::Assets* assets_;
    std::vector<Row> rows_;
    core::Connections connections_;
};

}
