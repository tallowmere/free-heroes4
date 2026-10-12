#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "h4/core/clock.hpp"
#include "h4/core/geometry.hpp"
#include "h4/graphics/text.hpp"
#include "h4/platform/input.hpp"

namespace h4::formats {
class Font;
}
namespace h4::graphics {
class Canvas;
}

namespace h4::ui {

class BalloonHelp {
public:
    static constexpr std::chrono::milliseconds kDelay{1000};

    explicit BalloonHelp(const formats::Font& font) noexcept : font_{&font} {
    }

    void input(const platform::InputEvent& event);

    template <class Lookup>
        requires std::is_invocable_r_v<std::string, Lookup&, core::Point>
    void advance(core::Instant now, core::Size root, Lookup&& lookup) {
        if (shown() || !pointer_)
            return;
        if (idle_.age(now) <= kDelay)
            return;
        const std::string words = std::invoke(lookup, *pointer_);
        if (!words.empty())
            show(words, *pointer_, root);
    }

    void paint(const graphics::Canvas& canvas) const;

    [[nodiscard]] bool shown() const noexcept {
        return !lines_.empty();
    }
    [[nodiscard]] core::Rect rect() const noexcept {
        return rect_;
    }
    [[nodiscard]] std::string_view text() const noexcept {
        return text_;
    }

private:
    void show(std::string_view text, core::Point pointer, core::Size root);

    const formats::Font* font_;
    std::optional<core::Point> pointer_;
    core::Since idle_;
    std::string text_;
    std::vector<std::string> lines_;
    core::Rect rect_;
    mutable graphics::WrappedText wrapped_;
};

}
