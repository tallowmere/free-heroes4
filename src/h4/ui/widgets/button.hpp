#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "h4/core/signal.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/graphics/text.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class ButtonFaces {
public:
    static constexpr std::array<std::string_view, 5> kStateNames{"Released", "Pressed", "Highlighted", "Highlighted_Pressed", "Disabled"};
    static constexpr char kJoin = '_';

    ButtonFaces(const formats::Layer& released,
                const formats::Layer* pressed,
                const formats::Layer* highlighted,
                const formats::Layer* highlighted_pressed,
                const formats::Layer* disabled) noexcept
    : released_{&released}, pressed_{pressed}, highlighted_{highlighted}, highlighted_pressed_{highlighted_pressed},
      disabled_{disabled} {
    }

    [[nodiscard]] static std::optional<ButtonFaces> from_group(const formats::BitmapGroup& group, std::string_view base);

    [[nodiscard]] const formats::Layer& released() const noexcept {
        return *released_;
    }
    [[nodiscard]] const formats::Layer* disabled() const noexcept {
        return disabled_;
    }
    [[nodiscard]] ButtonFaces with_disabled(const formats::Layer& face) const noexcept;
    [[nodiscard]] const formats::Layer& face(PressVisual visual, bool enabled) const noexcept;
    [[nodiscard]] const formats::Layer& toggle_face(bool pressed, bool hovered, bool enabled) const noexcept;
    [[nodiscard]] core::Rect extent() const noexcept;
    [[nodiscard]] core::Rect placed(core::Point anchor) const noexcept;
    [[nodiscard]] core::Point draw_origin() const noexcept;

private:
    const formats::Layer* released_;
    const formats::Layer* pressed_             = nullptr;
    const formats::Layer* highlighted_         = nullptr;
    const formats::Layer* highlighted_pressed_ = nullptr;
    const formats::Layer* disabled_            = nullptr;
};

struct ButtonCaption {
    std::string text;
    const formats::Font* font = nullptr;
    core::Rect released;
    core::Rect pressed;
    core::Rect disabled;
    graphics::TextStyle ink;
    graphics::TextStyle disabled_ink;
};

class Button final : public Widget {
public:
    Button(core::Point anchor, ButtonFaces faces);

    core::Signal<> clicked;

    [[nodiscard]] core::Rect art_bounds() const noexcept;

    void set_caption(ButtonCaption caption) {
        caption_ = std::move(caption);
    }

    void set_faces(ButtonFaces faces);

protected:
    void draw(const graphics::Canvas& canvas) const override;

    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return true;
    }
    void on_click() override {
        clicked.emit();
    }

private:
    ButtonFaces faces_;
    std::optional<ButtonCaption> caption_;
    mutable graphics::WrappedText wrapped_;
};

}
