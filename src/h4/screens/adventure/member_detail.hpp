#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/screens/adventure/meters.hpp"
#include "h4/screens/adventure/named_icon.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::formats {
class BitmapGroup;
class Layer;
}

namespace h4::screens {

class Builder;
class CreaturePortrait;

struct ArmyWindowMember {
    bool hero = true;
    std::string name, class_line;
    std::string text;
    std::string model, alignment;
    const formats::Layer* portrait = nullptr;
    std::string damage, hit_points, melee_attack, melee_defense, ranged_attack, ranged_defense, speed, move,
        spell_points, shots, experience, morale, luck;
    std::optional<int> morale_value;
    std::optional<int> luck_value;
    std::array<std::array<NamedIcon, 4>, 5> skills{};
    const formats::BitmapGroup* doll = nullptr;
    std::array<NamedIcon, 14> worn{};
    std::array<NamedIcon, 10> backpack{};
};

class MemberPage final : public Grouping {
public:
    MemberPage(Builder& window, const ArmyWindowMember& member);

    [[nodiscard]] core::Rect worn_rect(std::size_t slot) const noexcept;
    [[nodiscard]] core::Rect skill_rect(std::size_t row, std::size_t column) const noexcept;
    [[nodiscard]] const CreaturePortrait* creature_portrait() const noexcept {
        return creature_portrait_;
    }
    [[nodiscard]] CreaturePortrait* creature_portrait() noexcept {
        return creature_portrait_;
    }

private:
    void add_hero(Builder& page, const ArmyWindowMember& hero);
    void add_creature(Builder& page, const ArmyWindowMember& creature);
    void add_grid(Builder& page, const ArmyWindowMember& member, std::string_view frame);
    void add_portrait(Builder& page, const ArmyWindowMember& member);

    std::array<core::Rect, 14> worn_rects_{};
    std::array<std::array<core::Rect, 4>, 5> skill_rects_{};
    CreaturePortrait* creature_portrait_ = nullptr;
};

class MemberNumbers final : public Grouping {
public:
    MemberNumbers(Builder& window, const ArmyWindowMember& member);

    [[nodiscard]] std::string_view text_at(std::string_view guide) const noexcept;

private:
    struct Number {
        std::string_view icon, window, help, guide;
        std::string ArmyWindowMember::* words;
        bool hero_only;
        std::int32_t size;
    };

    static const std::array<Number, 13> kNumbers;

    void add_number(Builder& window, const Number& number, const ArmyWindowMember& member);
    void add_meter(Builder& window, Meter which, std::optional<int> value);
    void add_line(Builder& window, std::string_view guide, const std::string& words);

    struct Text {
        std::string guide;
        std::string words;
    };
    std::vector<Text> texts_;
};

}
