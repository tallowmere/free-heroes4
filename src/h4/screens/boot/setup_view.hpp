#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/view_factory.hpp"
#include "h4/screens/shell/tree_view.hpp"
#include "h4/screens/shell/view.hpp"

namespace h4::formats {
class Font;
}
namespace h4::ui {
class Label;
class Widget;
}

namespace h4::screens {

enum class SetupPrompt : std::uint8_t { drop, pick_folder };

class SetupView final : public WidgetView<View> {
public:
    static constexpr core::Size kFrame{640, 480};

    [[nodiscard]] static core::Result<std::unique_ptr<SetupView>>
    build(SetupPrompt prompt = SetupPrompt::drop, bool offer_sign_in = false);
    ~SetupView() override;
    SetupView(const SetupView&)            = delete;
    SetupView& operator=(const SetupView&) = delete;

    core::Signal<> browse_clicked;
    core::Signal<> sign_in_clicked;
    core::Signal<> quit_clicked;
    core::Signal<> cancel_clicked;

    void show_waiting(std::string message);
    void show_progress(std::size_t files_done, std::size_t files_total, std::uint64_t bytes_done, std::uint64_t bytes_total);
    void show_download(std::uint64_t bytes_done, std::uint64_t bytes_total);

    [[nodiscard]] bool extracting() const noexcept {
        return extracting_;
    }
    [[nodiscard]] bool downloading() const noexcept {
        return downloading_;
    }
    [[nodiscard]] std::string_view status() const noexcept;
    [[nodiscard]] std::int32_t percent() const noexcept {
        return percent_;
    }

    [[nodiscard]] core::Rect browse_rect() const noexcept;
    [[nodiscard]] core::Rect sign_in_rect() const noexcept;
    [[nodiscard]] core::Rect quit_rect() const noexcept;
    [[nodiscard]] core::Rect cancel_rect() const noexcept;

private:
    class Bar;
    friend ViewFactory;

    SetupView(core::Size frame, SetupPrompt prompt, bool offer_sign_in);
    [[nodiscard]] core::Result<void> compose();
    void show_busy(bool busy);

    SetupPrompt prompt_;
    bool offer_sign_in_ = false;
    std::unique_ptr<const formats::Font> font_;
    ui::Label* status_   = nullptr;
    Bar* bar_            = nullptr;
    ui::Widget* browse_  = nullptr;
    ui::Widget* sign_in_ = nullptr;
    ui::Widget* quit_    = nullptr;
    ui::Widget* cancel_  = nullptr;
    core::Connections connections_;
    bool extracting_      = false;
    bool downloading_     = false;
    std::int32_t percent_ = 0;
};

}
