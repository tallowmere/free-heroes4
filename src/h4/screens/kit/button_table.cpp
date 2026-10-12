#include "h4/screens/kit/button_table.hpp"

#include <string>

#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/composer.hpp"

namespace h4::screens {

void add_buttons(CompositionRules& rules, std::span<const ButtonSpec> table) {
    for (const ButtonSpec& spec : table) {
        if (spec.art != ButtonArt::composed) {
            rules.placed_by_screen.push_back(spec.guide);
            continue;
        }
        if (!spec.text_key.empty())
            rules.text_keys.emplace_back(spec.guide, spec.text_key);
        if (!spec.help_item.empty())
            rules.help_items.push_back({spec.guide, spec.help_window, spec.help_item});
    }
}

void Builder::captions(std::span<const CaptionSpec> table, const ui::LabelStyle& style) {
    for (const CaptionSpec& caption : table)
        label(caption.guide, text(caption.text_key), style, HelpDecision::from_parent());
}

ui::Button& Builder::button(const ButtonSpec& spec) {
    switch (spec.art) {
    case ButtonArt::composed: return button(spec.guide);
    case ButtonArt::authored: return button(spec, core::Point{});
    case ButtonArt::faces:
    case ButtonArt::large: break;
    }
    return button(spec, marker(spec.guide));
}

ui::Button& Builder::button(const ButtonSpec& spec, core::Point at) {
    if (spec.art == ButtonArt::composed)
        return button(spec.guide);
    const std::string stem      = name_stem(spec.guide, "button");
    const std::string_view item = spec.help_item.empty() ? std::string_view{stem} : spec.help_item;
    const HelpDecision said     = !spec.unread.empty() ? HelpDecision::unread(spec.unread) :
        spec.help_window.empty()                       ? help(item) :
                                                         help(spec.help_window, item);
    const std::string_view key  = spec.text_key.empty() ? std::string_view{stem} : spec.text_key;
    ui::Button& made            = spec.art == ButtonArt::large ? large_button(at, key, said) :
        spec.art == ButtonArt::authored                        ? add<ui::Button>(said, at, faces(spec.guide)) :
                                                                 control(at, faces(spec.faces, spec.face_base), said);
    made.set_name(std::string{spec.guide});
    return made;
}

}
