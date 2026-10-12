#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "h4/screens/adventure/named_icon.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
class Layer;
}
namespace h4::ui {
class IconCell;
class Label;
}

namespace h4::screens {

class Builder;
struct AdventureLayout;

struct StatEntry {
    bool shown = false;
    std::string name;
    std::string class_line;
    std::string health, damage, move, mana;
    std::array<NamedIcon, 8> icons{};
    std::optional<int> luck;
    std::optional<int> morale;
};

class StatStrip final : public Grouping {
public:
    StatStrip(Builder& root, const AdventureLayout& layout, const formats::Layer* plate);

    void set_entry(const StatEntry& entry);
    [[nodiscard]] bool built() const noexcept {
        return name_ != nullptr;
    }
    [[nodiscard]] std::string_view text(std::size_t which) const noexcept;

private:
    const assets::Assets* assets_;
    ui::Label* name_       = nullptr;
    ui::Label* class_line_ = nullptr;
    std::array<ui::Label*, 4> numbers_{};
    std::array<ui::IconCell*, 8> icons_{};
    ui::IconCell* luck_   = nullptr;
    ui::IconCell* morale_ = nullptr;
    const formats::BitmapGroup* luck_art_   = nullptr;
    const formats::BitmapGroup* morale_art_ = nullptr;
};

}
