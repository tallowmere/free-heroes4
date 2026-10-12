#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "h4/app/controller.hpp"
#include "h4/app/modal_slot.hpp"
#include "h4/core/geometry.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::app {

class MessageQueue {
public:
    MessageQueue(screens::PopupHost& host,
                 const assets::Assets& assets,
                 Reporter report,
                 std::function<core::Size()> frame,
                 std::function<void()> drained);

    void add(std::string words);
    void add(screens::BasicDialogContent content, screens::DialogPlacement placement);
    void show();
    void show_first();
    void clear() noexcept {
        notices_.clear();
    }
    void forget() noexcept {
        dialog_.forget();
    }

    [[nodiscard]] bool is_open() const noexcept {
        return dialog_.is_open();
    }
    [[nodiscard]] const screens::BasicDialog* dialog() const noexcept {
        return dialog_.get();
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return notices_.size();
    }
    bool on_key(platform::Key key) {
        return dialog_.on_key(key);
    }

private:
    struct Notice {
        screens::BasicDialogContent content;
        screens::DialogPlacement placement;
    };

    void read();

    const assets::Assets* assets_;
    Reporter report_;
    std::function<core::Size()> frame_;
    std::function<void()> drained_;
    std::vector<Notice> notices_;
    ModalSlot<screens::BasicDialog> dialog_;
};

}
