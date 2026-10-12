#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "h4/core/signal.hpp"
#include "h4/platform/input.hpp"
#include "h4/screens/shell/popup_host.hpp"

namespace h4::app {

struct DialogKeys {
    std::function<void()> escape;
    std::function<void()> enter;
};

template <class View> class ModalSlot {
public:
    explicit ModalSlot(screens::PopupHost& host) noexcept : host_{&host} {
    }
    ModalSlot(const ModalSlot&)            = delete;
    ModalSlot& operator=(const ModalSlot&) = delete;

    template <class Wire> void open(std::unique_ptr<View> view, Wire&& wire, DialogKeys keys = {}) {
        connections_.clear();
        view_ = view.get();
        keys_ = std::move(keys);
        std::forward<Wire>(wire)(*view_, connections_);
        host_->open(std::move(view));
    }
    void close() noexcept {
        if (is_open())
            host_->close();
        forget();
    }
    void forget() noexcept {
        view_ = nullptr;
        connections_.clear();
        keys_.escape = nullptr;
        keys_.enter  = nullptr;
    }
    bool on_key(platform::Key key) {
        if (!is_open())
            return false;
        const std::function<void()> run = key == platform::Key::escape ? keys_.escape :
            key == platform::Key::enter                                ? keys_.enter :
                                                                         nullptr;
        if (run)
            run();
        return true;
    }

    [[nodiscard]] bool is_open() const noexcept {
        return view_ != nullptr && host_->shows(*view_);
    }
    [[nodiscard]] View* get() const noexcept {
        return is_open() ? view_ : nullptr;
    }

private:
    screens::PopupHost* host_;
    View* view_ = nullptr;
    core::Connections connections_;
    DialogKeys keys_;
};

template <class View, class Controller> class ControlledSlot {
public:
    explicit ControlledSlot(screens::PopupHost& host) noexcept : slot_{host} {
    }
    ControlledSlot(const ControlledSlot&)            = delete;
    ControlledSlot& operator=(const ControlledSlot&) = delete;

    template <class Wire>
    void open(std::unique_ptr<View> view, std::unique_ptr<Controller> controller, Wire&& wire, DialogKeys keys = {}) {
        slot_.open(
            std::move(view),
            [&](View& opened, core::Connections& connections) {
                std::forward<Wire>(wire)(opened, *controller, connections);
            },
            std::move(keys)
        );
        controller_ = std::move(controller);
    }
    void close() noexcept {
        slot_.close();
        detach();
    }
    void forget() noexcept {
        slot_.forget();
        detach();
    }
    bool on_key(platform::Key key) {
        return slot_.on_key(key);
    }

    [[nodiscard]] bool is_open() const noexcept {
        return slot_.is_open();
    }
    [[nodiscard]] View* view() const noexcept {
        return slot_.get();
    }
    [[nodiscard]] Controller* controller() const noexcept {
        return controller_.get();
    }

private:
    void detach() noexcept {
        if (controller_)
            controller_->detach();
    }

    ModalSlot<View> slot_;
    std::unique_ptr<Controller> controller_;
};

}
