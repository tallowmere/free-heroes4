#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace h4::core {

namespace detail {
class SignalStateBase {
public:
    virtual ~SignalStateBase()                  = default;
    virtual void disconnect(std::uint64_t slot) = 0;
};
}

class Connection {
public:
    Connection() noexcept = default;
    Connection(std::weak_ptr<detail::SignalStateBase> state, std::uint64_t id) noexcept
    : state_{std::move(state)}, id_{id} {
    }

    Connection(Connection&& other) noexcept : state_{std::move(other.state_)}, id_{std::exchange(other.id_, 0)} {
    }
    Connection& operator=(Connection&& other) noexcept {
        if (this != &other) {
            disconnect();
            state_ = std::move(other.state_);
            id_    = std::exchange(other.id_, 0);
        }
        return *this;
    }
    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection() {
        disconnect();
    }

    void disconnect() noexcept {
        if (auto s = state_.lock())
            s->disconnect(id_);
        state_.reset();
        id_ = 0;
    }
    [[nodiscard]] bool connected() const noexcept {
        return !state_.expired() && id_ != 0;
    }

private:
    std::weak_ptr<detail::SignalStateBase> state_;
    std::uint64_t id_ = 0;
};

template <class... Args> class Signal {
public:
    using Slot = std::function<void(Args...)>;

    Signal()                         = default;
    Signal(const Signal&)            = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&)                 = delete;
    Signal& operator=(Signal&&)      = delete;

    [[nodiscard]] Connection connect(Slot slot) {
        const std::uint64_t id = state_->next_id++;
        state_->slots.push_back({id, std::make_shared<Slot>(std::move(slot))});
        return Connection{state_, id};
    }

    void emit(Args... args) const {
        auto state    = state_;
        auto snapshot = state->slots;
        for (const auto& entry : snapshot) {
            if (!state->is_connected(entry.id))
                continue;
            (*entry.slot)(args...);
        }
    }

    [[nodiscard]] std::size_t slot_count() const noexcept {
        return state_->slots.size();
    }

private:
    struct Entry {
        std::uint64_t id;
        std::shared_ptr<Slot> slot;
    };
    struct State final : detail::SignalStateBase {
        std::vector<Entry> slots;
        std::uint64_t next_id = 1;

        void disconnect(std::uint64_t id) override {
            std::erase_if(slots, [id](const Entry& e) { return e.id == id; });
        }
        [[nodiscard]] bool is_connected(std::uint64_t id) const {
            return std::ranges::any_of(slots, [id](const Entry& e) { return e.id == id; });
        }
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
};

class Connections {
public:
    Connections()                                  = default;
    Connections(Connections&&) noexcept            = default;
    Connections& operator=(Connections&&) noexcept = default;
    Connections(const Connections&)                = delete;
    Connections& operator=(const Connections&)     = delete;

    template <class... Args, class Slot> void on(Signal<Args...>& signal, Slot&& slot) {
        held_.push_back(signal.connect(std::forward<Slot>(slot)));
    }

    template <class... Source, class... Target, class... Bound>
    void relay(Signal<Source...>& source, Signal<Target...>& target, Bound... bound) {
        on(source, [&target, bound...](Source... args) { target.emit(bound..., args...); });
    }

    void clear() noexcept {
        held_.clear();
    }

    void keep(Connection connection) {
        held_.push_back(std::move(connection));
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return held_.size();
    }

private:
    std::vector<Connection> held_;
};

}
