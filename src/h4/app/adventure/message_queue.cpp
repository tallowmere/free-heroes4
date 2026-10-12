#include "h4/app/adventure/message_queue.hpp"

#include <utility>

namespace h4::app {

MessageQueue::MessageQueue(screens::PopupHost& host,
                           const assets::Assets& assets,
                           Reporter report,
                           std::function<core::Size()> frame,
                           std::function<void()> drained)
: assets_{&assets}, report_{std::move(report)}, frame_{std::move(frame)}, drained_{std::move(drained)}, dialog_{host} {
}

void MessageQueue::add(std::string words) {
    notices_.push_back({screens::BasicDialogContent{.text = std::move(words)}, {}});
}

void MessageQueue::add(screens::BasicDialogContent content, screens::DialogPlacement placement) {
    notices_.push_back({std::move(content), placement});
}

void MessageQueue::show() {
    if (!dialog_.is_open())
        show_first();
}

void MessageQueue::show_first() {
    if (notices_.empty()) {
        dialog_.close();
        if (drained_)
            drained_();
        return;
    }
    auto dialog = screens::BasicDialog::build(*assets_, frame_(), notices_.front().content, notices_.front().placement);
    if (report_.failed(dialog)) {
        notices_.clear();
        return;
    }
    dialog_.open(std::move(*dialog),
                 [this](screens::BasicDialog& message, core::Connections& wiring) {
                     wiring.on(message.dismissed, [this] { read(); });
                 },
                 {[this] { read(); }, {}});
}

void MessageQueue::read() {
    if (!notices_.empty())
        notices_.erase(notices_.begin());
    show_first();
}

}
