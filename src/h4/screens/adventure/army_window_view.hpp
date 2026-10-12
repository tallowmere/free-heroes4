#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/adventure/army_rings_view.hpp"
#include "h4/screens/adventure/hero_list_view.hpp"
#include "h4/screens/adventure/member_detail.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widgets/radio_group.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::ui {
class Button;
}

namespace h4::screens {

class CreaturePortrait;

struct ArmyWindowModel {
    std::array<ArmyRingEntry, kArmyRings> army{};
    std::optional<std::array<ArmyRingEntry, kArmyRings>> below;
    std::optional<std::size_t> selected;
    std::vector<HeroListEntry> armies;
    std::optional<std::size_t> army_row;
    std::optional<ArmyWindowMember> member;
};

class ArmyWindowView final : public ModalPopup {
    friend ViewFactory;
    ArmyWindowView(core::Size frame, ArmyWindowModel model) : ModalPopup{frame}, model_{std::move(model)} {
    }

public:
    enum class Control : std::uint8_t {
        dismiss,
        split,
        spellbook,
        hide_potions,
        loose,
        tight,
        square,
        move_army_up,
        move_army_down
    };

    [[nodiscard]] static core::Result<std::unique_ptr<ArmyWindowView>>
    build(const assets::Assets& assets, core::Size frame, ArmyWindowModel model);

    core::Signal<> closed;
    core::Signal<std::size_t> ring_pressed;
    core::Signal<Control> button_clicked;
    core::Signal<std::size_t> army_pressed;

    void advance(core::Instant now) override;

    [[nodiscard]] const ArmyWindowModel& content() const noexcept {
        return model_;
    }
    [[nodiscard]] core::Rect ok_rect() const noexcept;
    [[nodiscard]] core::Rect control_rect(Control control) const noexcept;
    [[nodiscard]] const ArmyRingsView& rings() const noexcept {
        return *rings_;
    }
    [[nodiscard]] const HeroListView& army_list() const noexcept {
        return *army_list_;
    }
    [[nodiscard]] const CreaturePortrait* creature_portrait() const noexcept {
        return page_ != nullptr ? page_->creature_portrait() : nullptr;
    }
    [[nodiscard]] std::string_view text_at(std::string_view guide) const noexcept {
        return numbers_ != nullptr ? numbers_->text_at(guide) : std::string_view{};
    }
    [[nodiscard]] core::Rect worn_rect(std::size_t slot) const noexcept {
        return page_ != nullptr ? page_->worn_rect(slot) : core::Rect{};
    }
    [[nodiscard]] core::Rect skill_rect(std::size_t row, std::size_t column) const noexcept {
        return page_ != nullptr ? page_->skill_rect(row, column) : core::Rect{};
    }

private:
    [[nodiscard]] core::Result<void> compose(const assets::Assets& assets);
    void add_buttons(Builder& window);
    void add_formations(Builder& window);
    void announce(ui::Button& button, Control control);
    void add_rings(Builder& window);
    void add_army_list(Builder& window);

    ArmyWindowModel model_;
    ui::Button* ok_ = nullptr;
    std::array<ui::Button*, 9> controls_{};
    std::unique_ptr<ui::RadioGroup> formations_;
    ArmyRingsView* rings_    = nullptr;
    ArmyRingsView* below_    = nullptr;
    HeroListView* army_list_ = nullptr;
    MemberPage* page_        = nullptr;
    MemberNumbers* numbers_  = nullptr;
    core::Connections connections_;
};

}
