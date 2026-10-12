#include "h4/screens/kit/large_button.hpp"

#include <optional>
#include <string>

#include "h4/assets/assets.hpp"
#include "h4/assets/required.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/graphics/text.hpp"
#include "h4/screens/kit/control.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/button.hpp"

namespace h4::screens {

namespace {
constexpr graphics::Rgba kDisabledInk{99, 101, 99, 255};
}

core::Result<ui::Button*>
add_large_button(ui::Widget& parent, const assets::Assets& assets, core::Point guide_origin, std::string_view text_key, const HelpDecision& help) {
    const auto faces = assets::require_art<ui::ButtonFaces>(assets, "large_button", "");
    const auto guides = assets::require_layers(assets, "large_button", {"released_text", "pressed_text", "disabled_text"});
    if (auto failed = core::first_error(faces, guides))
        return std::unexpected{*failed};
    const auto [released, pressed, disabled] = *guides;
    const auto caption_font                  = assets::require_font(assets, released->bounds().height());
    if (!caption_font)
        return std::unexpected{caption_font.error().with_context("large_button")};
    const formats::Font* font = *caption_font;

    auto& button = add_control(parent, help, *faces, guide_origin);
    const std::string caption{assets.text(text_key)};
    button.set_caption({caption,
                        font,
                        released->bounds(),
                        pressed->bounds(),
                        disabled->bounds(),
                        graphics::TextStyle::engraved(),
                        graphics::TextStyle{kDisabledInk, std::nullopt}});
    return &button;
}

}
