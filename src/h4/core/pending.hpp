#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <optional>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>

namespace h4::core {

template <class T> class Pending {
public:
    Pending() = default;

    template <class Work>
        requires std::is_invocable_r_v<T, Work&, std::stop_token>
    [[nodiscard]] static Pending run(Work work) {
        Pending out;
        std::promise<T> promise;
        out.value_ = promise.get_future();
        out.thread_ = std::jthread{[promise = std::move(promise), work = std::move(work)](std::stop_token stop) mutable {
            try {
                promise.set_value(work(stop));
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        }};
        return out;
    }

    Pending(Pending&&) noexcept            = default;
    Pending& operator=(Pending&&) noexcept = default;
    Pending(const Pending&)                = delete;
    Pending& operator=(const Pending&)     = delete;
    ~Pending() = default;

    [[nodiscard]] bool started() const noexcept {
        return value_.valid();
    }
    [[nodiscard]] bool ready() const {
        return value_.valid() && value_.wait_for(std::chrono::seconds{0}) == std::future_status::ready;
    }
    [[nodiscard]] std::optional<T> take() {
        if (!ready())
            return std::nullopt;
        std::optional<T> out{value_.get()};
        if (thread_.joinable())
            thread_.join();
        return out;
    }

    template <class Start>
        requires std::is_same_v<std::invoke_result_t<Start&>, Pending>
    [[nodiscard]] std::optional<T> start_or_take(Start start) {
        if (!started())
            *this = start();
        return take();
    }

private:
    std::future<T> value_;
    std::jthread thread_;
};

}
