#pragma once

#include <memory>

#include "h4/screens/shell/popup.hpp"
#include "h4/screens/shell/view.hpp"

namespace h4::screens {

class PopupHost final : public View {
public:
    explicit PopupHost(View& base) noexcept : base_{&base} {
    }

    PopupHost(const PopupHost&)            = delete;
    PopupHost& operator=(const PopupHost&) = delete;

    void open(std::unique_ptr<Popup> popup);
    void close() noexcept;

    void set_base(View& base) noexcept;
    template <class V> void rebase(std::unique_ptr<V>& slot, std::unique_ptr<V> next) noexcept {
        slot = std::move(next);
        set_base(*slot);
    }

    [[nodiscard]] bool has_popup() const noexcept;
    [[nodiscard]] bool shows(const Popup& popup) const noexcept;

    void handle(const platform::InputEvent& event) override;
    void paint(const graphics::Canvas& canvas) const override;
    void advance(core::Instant now) override;
    [[nodiscard]] cursor::CursorState cursor_state() const override;
    [[nodiscard]] ui::HelpAt help_at(core::Point point) const override;

private:
    void dispatch(const platform::InputEvent& event);
    void settle() noexcept;

    View* base_;
    std::unique_ptr<Popup> popup_;
    std::unique_ptr<Popup> incoming_;
    bool dispatching_   = false;
    bool close_pending_ = false;
};

}
