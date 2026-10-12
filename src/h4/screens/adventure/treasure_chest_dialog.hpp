#pragma once

#include <array>
#include <memory>
#include <string>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/widget_tree.hpp"
#include "h4/ui/widgets/radio_group.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::ui {
class Button;
class ButtonFaces;
}

namespace h4::screens {

class Builder;

struct TreasureChestContent {
    std::string title;
    std::string text;
    std::string gold_label;
    std::string experience_label;
};

class TreasureChestDialog final : public ModalPopup {
public:
    enum class Choice : std::uint8_t { gold, experience };

    [[nodiscard]] static core::Result<std::unique_ptr<TreasureChestDialog>>
    build(const assets::Assets& assets, core::Size frame, TreasureChestContent content);

    ~TreasureChestDialog() override;

    core::Signal<Choice> chosen;

    [[nodiscard]] Choice choice() const noexcept;
    void choose(Choice choice);
    [[nodiscard]] const TreasureChestContent& content() const noexcept {
        return content_;
    }
    [[nodiscard]] core::Rect button_rect() const noexcept;
    [[nodiscard]] core::Rect choice_rect(Choice choice) const noexcept;

private:
    friend ViewFactory;

    TreasureChestDialog(core::Size frame, TreasureChestContent content)
    : ModalPopup{frame}, content_{std::move(content)} {
    }

    [[nodiscard]] core::Result<void> compose(const assets::Assets& assets);
    void add_choices(Builder& dialog, const ui::ButtonFaces& frame, core::Point icon);

    TreasureChestContent content_;
    std::array<core::Rect, 2> choice_rects_{};
    ui::Button* ok_ = nullptr;
    std::unique_ptr<ui::RadioGroup> choices_;
    core::Connections connections_;
};

}
