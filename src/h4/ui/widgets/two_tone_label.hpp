#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "h4/core/signal.hpp"
#include "h4/formats/font.hpp"
#include "h4/ui/widget.hpp"
#include "h4/ui/widgets/blend_window.hpp"
#include "h4/ui/widgets/label.hpp"

namespace h4::ui {

class BlendWindow;

class TwoToneLabel final : public Widget {
public:
    enum class Mode : std::uint8_t { hover, manual };

    TwoToneLabel(core::Rect rect, std::string text, const formats::Font& font, LabelStyle plain, LabelStyle washed, std::optional<Wash> wash, Mode mode);

    core::Signal<> clicked;

    void set_text(std::string text);
    void set_washed(bool washed);

    [[nodiscard]] bool washed() const noexcept {
        return washed_;
    }

protected:
    [[nodiscard]] bool accepts_pointer() const noexcept override {
        return mode_ == Mode::hover;
    }
    void on_state_changed() override;
    void on_click() override {
        clicked.emit();
    }

private:
    void refresh();

    Label* plain_      = nullptr;
    BlendWindow* wash_ = nullptr;
    Label* hot_        = nullptr;
    Mode mode_;
    bool washed_ = false;
};

}
