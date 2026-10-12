#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace h4::platform::sdl3 {

class Inbox final {
public:
    void push(std::string item) {
        const std::scoped_lock lock{mutex_};
        items_.push_back(std::move(item));
    }

    [[nodiscard]] std::vector<std::string> take() {
        const std::scoped_lock lock{mutex_};
        return std::exchange(items_, {});
    }

    void keep(std::shared_ptr<const void> held) {
        const std::scoped_lock lock{mutex_};
        kept_.push_back(std::move(held));
    }

private:
    std::mutex mutex_;
    std::vector<std::string> items_;
    std::vector<std::shared_ptr<const void>> kept_;
};

}
