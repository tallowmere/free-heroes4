#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "h4/core/clock.hpp"
#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"
#include "h4/platform/input.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::model {
struct GameSettings;
}

namespace h4::screens {
class View;
}

namespace h4::app {

class Controller;

namespace transitions {
struct Quit {};
struct Switch {
    std::unique_ptr<Controller> next;
};
}

class Reporter {
public:
    Reporter() = default;
    template <class F>
        requires(!std::is_same_v<std::decay_t<F>, Reporter> && std::is_invocable_v<F&, std::string_view>)
    Reporter(F report) : report_{std::move(report)} {
    }

    void operator()(std::string_view message) const {
        if (report_)
            report_(message);
    }
    void not_ported(std::string_view what) const;
    template <class T> [[nodiscard]] bool failed(const core::Result<T>& result) const {
        if (result)
            return false;
        (*this)(result.error().message());
        return true;
    }

private:
    std::function<void(std::string_view)> report_;
};

using SettingsSaver = std::function<void(const model::GameSettings& settings)>;

using Transition = std::variant<transitions::Quit, transitions::Switch>;

struct FramePolicy {
    bool follow_window        = true;
    screens::Resolution fixed = screens::Resolution::r1024x768;
};

class Controller {
public:
    virtual ~Controller();

    [[nodiscard]] virtual screens::View& view() noexcept = 0;

    virtual void on_key(platform::Key ) {
    }

    virtual void on_file_dropped(const std::string& ) {
    }

    virtual void on_web_sign_in(const std::string& ) {
    }

    void set_frame_policy(FramePolicy policy) noexcept {
        frame_policy_ = policy;
    }
    [[nodiscard]] FramePolicy frame_policy() const noexcept {
        return frame_policy_;
    }

    [[nodiscard]] virtual core::Size preferred_frame(core::Size available) const {
        if (!frame_policy_.follow_window)
            return screens::size_of(frame_policy_.fixed);
        return {screens::size_of(screens::variant_for_height(available.height)).width, available.height};
    }

    virtual void on_resized(core::Size ) {
    }

    virtual void advance(core::Instant ) {
    }

    [[nodiscard]] std::optional<Transition> take_transition() {
        on_frame();
        return std::exchange(pending_, std::nullopt);
    }

protected:
    virtual void on_frame() {
    }
    void request(Transition next) {
        pending_ = std::move(next);
    }

    Controller()                             = default;
    Controller(const Controller&)            = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&)                 = default;
    Controller& operator=(Controller&&)      = default;

private:
    FramePolicy frame_policy_;
    std::optional<Transition> pending_;
};

}
