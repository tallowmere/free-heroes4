#include "h4/screens/kit/window_builder.hpp"

#include <string>

#include "h4/assets/assets.hpp"

namespace h4::screens {

HelpDecision HelpDecision::row(const assets::Assets& assets, std::string_view window, std::string_view item) {
    ui::Help help = interface_help(assets, window, item);
    if (help.empty())
        return unread("a table.Interface row the table lacks");
    return HelpDecision{std::move(help)};
}

void HelpDecision::apply(ui::Widget& window) const {
    if (const auto* help = std::get_if<ui::Help>(&choice_))
        window.set_help(*help);
    else if (const auto* source = std::get_if<ui::HelpFunction>(&choice_))
        window.set_help(*source);
    else if (const auto* silent = std::get_if<Silent>(&choice_))
        window.declare_silent(silent->kind, silent->reason);
}

ui::Help interface_help(const assets::Assets& assets, std::string_view window, std::string_view item) {
    const assets::InterfaceHelp row = assets.help(window, item);
    return {std::string{row.balloon}, std::string{row.right_click}};
}

}
