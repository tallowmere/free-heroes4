#pragma once

#include <string_view>
#include <utility>
#include <variant>

#include "h4/ui/help.hpp"
#include "h4/ui/widget.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::screens {

class HelpDecision {
public:
    [[nodiscard]] static HelpDecision row(const assets::Assets& assets, std::string_view window, std::string_view item);
    [[nodiscard]] static HelpDecision words(ui::Help help) {
        return HelpDecision{std::move(help)};
    }
    [[nodiscard]] static HelpDecision source(ui::HelpFunction source) {
        return HelpDecision{std::move(source)};
    }
    [[nodiscard]] static HelpDecision from_parent() {
        return HelpDecision{Parent{}};
    }
    [[nodiscard]] static HelpDecision silent(std::string_view reason) {
        return HelpDecision{Silent{ui::Silence::retail, reason}};
    }
    [[nodiscard]] static HelpDecision unread(std::string_view what) {
        return HelpDecision{Silent{ui::Silence::unread, what}};
    }

    void apply(ui::Widget& window) const;

private:
    struct Parent {};
    struct Silent {
        ui::Silence kind;
        std::string_view reason;
    };
    using Choice = std::variant<ui::Help, ui::HelpFunction, Parent, Silent>;

    explicit HelpDecision(Choice choice) : choice_{std::move(choice)} {
    }

    Choice choice_;
};

template <class T, class... Args> T& add_window(ui::Widget& parent, const HelpDecision& help, Args&&... args) {
    T& window = parent.emplace_child<T>(std::forward<Args>(args)...);
    help.apply(window);
    return window;
}

[[nodiscard]] ui::Help interface_help(const assets::Assets& assets, std::string_view window, std::string_view item);

}
