#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/ui/help.hpp"
#include "h4/ui/widget_tree.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Layer;
}
namespace h4::ui {
class Button;
}

namespace h4::screens {

class Builder;
namespace detail {
struct DialogArt;
struct DialogGeometry;
}

struct DialogItem {
    struct Piece {
        const formats::Layer* layer = nullptr;
        core::Point at;
    };
    std::vector<Piece> pieces;
    std::string caption;
    ui::Help help;
};

enum class DialogButtons : std::uint8_t { check, check_and_cross, none };

struct BasicDialogContent {
    std::string title;
    std::string text;
    bool centred = false;
    std::vector<DialogItem> items;
    DialogButtons buttons = DialogButtons::check;
};

struct DialogPlacement {
    enum class Anchor : std::uint8_t { centred, standing_on, hanging_from };
    Anchor anchor = Anchor::centred;
    core::Point at{};

    [[nodiscard]] static constexpr DialogPlacement standing_on(core::Point point) noexcept {
        return {Anchor::standing_on, point};
    }
    [[nodiscard]] static constexpr DialogPlacement hanging_from(core::Point point) noexcept {
        return {Anchor::hanging_from, point};
    }
};

class BasicDialog final : public ModalPopup {
public:
    [[nodiscard]] static core::Result<std::unique_ptr<BasicDialog>>
    build(const assets::Assets& assets, core::Size frame, BasicDialogContent content, DialogPlacement placement = {});

    core::Signal<> dismissed;
    core::Signal<bool> answered;

    [[nodiscard]] std::string_view text() const noexcept {
        return content_.text;
    }
    [[nodiscard]] std::string_view title() const noexcept {
        return content_.title;
    }
    [[nodiscard]] bool question() const noexcept {
        return content_.buttons == DialogButtons::check_and_cross;
    }
    [[nodiscard]] core::Rect button_rect() const noexcept;
    [[nodiscard]] core::Rect no_rect() const noexcept;
    [[nodiscard]] core::Rect scroll_rect() const noexcept {
        return scroll_rect_;
    }
    [[nodiscard]] core::Rect item_rect(std::size_t index) const noexcept;

private:
    friend ViewFactory;

    BasicDialog(core::Size frame, BasicDialogContent content, DialogPlacement placement)
    : ModalPopup{frame}, content_{std::move(content)}, placement_{placement} {
    }

    [[nodiscard]] core::Result<void> compose(const assets::Assets& assets);
    void add_title(const detail::DialogArt& art, const detail::DialogGeometry& layout);
    void add_words(const detail::DialogArt& art, const detail::DialogGeometry& layout);
    void add_items(const detail::DialogArt& art, const detail::DialogGeometry& layout);
    void add_buttons(Builder& dialog, const detail::DialogArt& art, const detail::DialogGeometry& layout);

    BasicDialogContent content_;
    DialogPlacement placement_;
    ui::Button* ok_ = nullptr;
    ui::Button* no_ = nullptr;
    core::Rect scroll_rect_;
    std::vector<core::Rect> item_rects_;
    core::Connections connections_;
};

}
