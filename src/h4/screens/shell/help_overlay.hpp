#pragma once

#include <chrono>
#include <memory>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/platform/input.hpp"
#include "h4/ui/balloon_help.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class Font;
}
namespace h4::graphics {
class Canvas;
}

namespace h4::screens {

class BasicDialog;
class View;

class HelpOverlay final {
public:
    [[nodiscard]] static core::Result<HelpOverlay> build(const assets::Assets& assets);

    HelpOverlay(const assets::Assets& assets, const formats::Font& balloon_font);

    HelpOverlay(HelpOverlay&&) noexcept;
    HelpOverlay& operator=(HelpOverlay&&) noexcept;
    ~HelpOverlay();

    [[nodiscard]] bool admit(const platform::InputEvent& event);
    void settle(const platform::InputEvent& event, const View& view, core::Size root);
    void advance(core::Instant now, const View& view, core::Size root);
    void close() noexcept;

    void paint(const graphics::Canvas& canvas) const;

    [[nodiscard]] const ui::BalloonHelp& balloon() const noexcept {
        return balloon_;
    }
    [[nodiscard]] const BasicDialog* popup() const noexcept {
        return popup_.get();
    }

private:
    const assets::Assets* assets_;
    ui::BalloonHelp balloon_;
    std::unique_ptr<BasicDialog> popup_;
};

}
