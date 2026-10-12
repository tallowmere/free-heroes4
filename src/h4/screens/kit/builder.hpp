#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/button_table.hpp"
#include "h4/screens/kit/composer.hpp"
#include "h4/screens/kit/control.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/dialog_frame.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/scrollbar.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
class Font;
class Layer;
}
namespace h4::ui {
class Sprite;
}

namespace h4::screens {

struct BuildSource {
    std::string_view group;
    std::string_view help_window;
    std::string_view screen;
};

class Builder {
public:
    [[nodiscard]] static core::Result<Builder> open(ui::Widget& parent, const assets::Assets& assets, BuildSource source);

    [[nodiscard]] core::Result<void> finish() const;
    [[nodiscard]] bool ok() const noexcept;
    void fail(core::Error error);

    [[nodiscard]] ui::Widget& parent() const noexcept {
        return *parent_;
    }
    [[nodiscard]] const assets::Assets& assets() const noexcept;
    [[nodiscard]] const formats::BitmapGroup& group() const noexcept;
    [[nodiscard]] const formats::BitmapGroup& group(std::string_view name);

    [[nodiscard]] Builder within(ui::Widget& parent, core::Point origin) const;
    [[nodiscard]] Builder inside(ui::Widget& child) const;
    [[nodiscard]] Builder within(ui::Widget& parent, std::string_view guide);
    [[nodiscard]] Builder over(std::string_view group, ui::Widget& parent, core::Point origin = {});
    [[nodiscard]] Builder panel(core::Rect box, const HelpDecision& help);
    [[nodiscard]] Builder centred(std::string_view name, const HelpDecision& help, std::optional<core::Size> size = {});

    [[nodiscard]] core::Rect guide(std::string_view name);
    [[nodiscard]] core::Point marker(std::string_view name) {
        return guide(name).origin();
    }
    [[nodiscard]] const formats::Layer& layer(std::string_view name);
    [[nodiscard]] const formats::Layer& layer(std::string_view group, std::string_view name);
    [[nodiscard]] const formats::Layer* find(std::string_view name) const;
    [[nodiscard]] const formats::Font& font(std::int32_t size);
    [[nodiscard]] ui::ButtonFaces faces(std::string_view base);
    [[nodiscard]] ui::ButtonFaces faces(std::string_view group, std::string_view base);
    [[nodiscard]] ui::ScrollbarArt scrollbar_art(std::string_view group);
    [[nodiscard]] ui::DialogFrameArt frame_art(std::string_view group);
    [[nodiscard]] HelpDecision help(std::string_view item) const;
    [[nodiscard]] HelpDecision help(std::string_view window, std::string_view item) const;
    [[nodiscard]] std::string text(std::string_view key) const;

    void compose(const CompositionRules& rules);
    [[nodiscard]] ui::Button& button(std::string_view name);
    ui::Button& button(const ButtonSpec& spec);
    ui::Button& button(const ButtonSpec& spec, core::Point at);
    template <class Id, std::size_t N>
    void buttons(const std::array<ButtonSpec, N>& table,
                 std::array<ui::Button*, N>& out,
                 core::Connections& connections,
                 core::Signal<Id>& clicked) {
        for (std::size_t i = 0; i < N; ++i) {
            out[i] = &button(table[i]);
            connections.relay(out[i]->clicked, clicked, static_cast<Id>(i));
        }
    }
    [[nodiscard]] ui::Label& text_window(std::string_view name);

    void captions(std::span<const CaptionSpec> table, const ui::LabelStyle& style);
    ui::Label&
    label(std::string_view guide, std::string words, const ui::LabelStyle& style, const HelpDecision& help, std::int32_t font_size = 0);
    ui::Label&
    label(core::Rect box, std::string words, const ui::LabelStyle& style, const HelpDecision& help, std::int32_t font_size = 0);

    template <class T = ui::Button, class... Args>
    T& control(std::string_view guide, const ui::ButtonFaces& faces, const HelpDecision& help, Args&&... args) {
        return control<T>(marker(guide), faces, help, std::forward<Args>(args)...);
    }
    template <class T = ui::Button, class... Args>
    T& control(core::Point marker, const ui::ButtonFaces& faces, const HelpDecision& help, Args&&... args) {
        return add_control<T>(*parent_, help, faces, marker, std::forward<Args>(args)...);
    }

    ui::Button& large_button(std::string_view guide, std::string_view text_key, const HelpDecision& help);
    ui::Button& large_button(core::Point at, std::string_view text_key, const HelpDecision& help);

    template <class Where>
    ui::ToggleButton& toggle(Where&& where, const ui::ButtonFaces& faces, const HelpDecision& help, bool checked = false) {
        return control<ui::ToggleButton>(std::forward<Where>(where), faces, help, checked);
    }

    ui::Sprite& sprite(std::string_view layer, const HelpDecision& help);
    ui::Sprite& sprite(const formats::Layer& layer, const HelpDecision& help);
    ui::Sprite& sprite_at(std::string_view guide, const formats::Layer& layer, const HelpDecision& help) {
        return sprite_at(marker(guide), layer, help);
    }
    ui::Sprite& sprite_at(core::Point marker, const formats::Layer& layer, const HelpDecision& help) {
        return add_control(*parent_, help, layer, marker);
    }

    template <class T, class... Args> T& add(const HelpDecision& help, Args&&... args) {
        return add_window<T>(*parent_, help, std::forward<Args>(args)...);
    }

private:
    struct State;

    Builder(std::shared_ptr<State> state, ui::Widget& parent, core::Point origin, const formats::BitmapGroup& group, std::string_view group_name) noexcept
    : state_{std::move(state)}, parent_{&parent}, origin_{origin}, group_{&group}, group_name_{group_name} {
    }

    std::shared_ptr<State> state_;
    ui::Widget* parent_;
    core::Point origin_;
    const formats::BitmapGroup* group_;
    std::string group_name_;
};

}
