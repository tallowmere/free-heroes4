#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/graphics/text.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
class Layer;
}
namespace h4::ui {
class Button;
class Label;
class Sprite;
}

namespace h4::screens {

using NameMap = std::vector<std::pair<std::string_view, std::string_view>>;

struct HelpEntry {
    std::string_view layer;
    std::string_view window;
    std::string_view item;
};

struct CompositionRules {
    std::string_view help_window;
    std::vector<std::string_view> placed_by_screen;
    NameMap text_keys;
    std::vector<HelpEntry> help_items;
    graphics::TextAlignment text_alignment{graphics::HorizontalAlign::centre, graphics::VerticalAlign::centre};
};

class Composition {
public:
    [[nodiscard]] ui::Button* button(std::string_view name) const noexcept;
    [[nodiscard]] ui::Label* label(std::string_view name) const noexcept;
    [[nodiscard]] ui::Sprite* sprite(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<core::Rect> guide(std::string_view name) const;

private:
    friend core::Result<Composition>
    compose(ui::Widget& parent, const assets::Assets& assets, const formats::BitmapGroup& group, const CompositionRules& rules);

    core::FoldedMap<ui::Button*> buttons_;
    core::FoldedMap<ui::Label*> labels_;
    core::FoldedMap<ui::Sprite*> sprites_;
    core::FoldedMap<core::Rect> guides_;
};

[[nodiscard]] std::string name_stem(std::string_view name, std::string_view suffix);

[[nodiscard]] core::Result<Composition>
compose(ui::Widget& parent, const assets::Assets& assets, const formats::BitmapGroup& group, const CompositionRules& rules);

[[nodiscard]] core::Result<ui::Label*> add_label(ui::Widget& parent,
                                                 const assets::Assets& assets,
                                                 const core::Rect& box,
                                                 std::string words,
                                                 graphics::TextAlignment alignment,
                                                 const HelpDecision& help,
                                                 std::int32_t font_size = 0);

}
