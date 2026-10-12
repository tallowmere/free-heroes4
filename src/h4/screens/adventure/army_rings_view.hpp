#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/ui/help.hpp"
#include "h4/ui/widget.hpp"

namespace h4::formats {
class Font;
class Layer;
}
namespace h4::ui {
class ClickArea;
class IconCell;
class Label;
class Meter;
class Panel;
}

namespace h4::screens {

class Builder;

struct ArmyRingEntry {
    enum class Kind : std::uint8_t { empty, creature, hero };
    Kind kind                  = Kind::empty;
    const formats::Layer* face = nullptr;
    std::string count;
    int health = 0, health_max = 0;
    int mana = 0, mana_max = 0;
    ui::Help help;
};

inline constexpr std::size_t kArmyRings = 7;

enum class RingLayout : std::uint8_t { middle_row, top_row, bottom_row, panel };

class ArmyRingsView final : public ui::Widget {
public:
    ArmyRingsView(Builder& builder, core::Point anchor, RingLayout layout = RingLayout::panel);

    void set_entries(std::span<const ArmyRingEntry> entries);
    void select(std::optional<std::size_t> slot);

    [[nodiscard]] core::Rect rings_rect() const noexcept;
    [[nodiscard]] core::Rect ring_rect(std::size_t slot) const noexcept;
    [[nodiscard]] std::string_view count_text(std::size_t slot) const noexcept;

    core::Signal<std::size_t> ring_pressed;
    core::Signal<std::size_t> ring_double_clicked;

private:
    struct Slot {
        ui::ClickArea* button   = nullptr;
        ui::IconCell* portrait  = nullptr;
        ui::Panel* inset        = nullptr;
        ui::Label* count        = nullptr;
        ui::Meter* health       = nullptr;
        ui::Meter* mana         = nullptr;
        ui::Widget* health_edge = nullptr;
        ui::Widget* mana_edge   = nullptr;
        ui::Widget* plain       = nullptr;
        ui::Widget* highlight   = nullptr;
    };

    struct BannerArt {
        const formats::Layer* inset;
        const formats::Layer* text;
        const formats::Layer* health;
        const formats::Layer* health_edge;
        const formats::Layer* mana;
        const formats::Layer* mana_edge;
        const formats::Font* font;

        [[nodiscard]] static BannerArt load(Builder& art);
    };

    void add_ring(Builder& art, std::size_t index, std::string_view shape, core::Point offset);
    void add_banner(const BannerArt& art, std::size_t index, core::Point offset);
    void fit(core::Point anchor);

    std::array<Slot, kArmyRings> slots_{};
    core::Size rings_size_;
    core::Connections connections_;
};

}
