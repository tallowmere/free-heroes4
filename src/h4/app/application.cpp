#include "h4/app/application.hpp"

#include <variant>

#include "h4/graphics/canvas.hpp"
#include "h4/screens/shell/resolution.hpp"
#include "h4/screens/shell/view.hpp"

namespace h4::app {

Application::Application(platform::Window& window,
                         cursor::CursorSource& cursors,
                         std::unique_ptr<Controller> initial,
                         std::optional<screens::HelpOverlay> help)
: window_{&window}, cursors_{&cursors}, controller_{std::move(initial)}, frame_{window.logical_size()},
  help_{std::move(help)} {
}

void Application::run() {
    using clock = std::chrono::steady_clock;
    auto last   = clock::now();
    for (;;) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - last);
        last += elapsed;
        if (!step(elapsed))
            return;
    }
}

void Application::resize_to_window() {
    const core::Size wanted = controller_->preferred_frame(screens::logical_frame(window_->window_size()));
    if (wanted.empty() || wanted == frame_.size())
        return;
    window_->set_logical_size(wanted);
    frame_ = graphics::Surface565{wanted};
    controller_->on_resized(wanted);
}

bool Application::step(std::chrono::milliseconds elapsed) {
    resize_to_window();
    if (!dispatch_input())
        return false;
    advance(elapsed);
    render();
    return take_transition();
}

bool Application::dispatch_input() {
    for (const platform::InputEvent& event : window_->poll_events()) {
        if (std::holds_alternative<platform::events::QuitRequested>(event))
            return false;
        if (help_ && !help_->admit(event))
            continue;
        if (const auto* key = std::get_if<platform::events::KeyPressed>(&event))
            controller_->on_key(key->key);
        else if (const auto* drop = std::get_if<platform::events::FileDropped>(&event))
            controller_->on_file_dropped(drop->path);
        else if (const auto* signed_in = std::get_if<platform::events::WebSignInFinished>(&event))
            controller_->on_web_sign_in(signed_in->landing);
        else {
            screens::View& view = controller_->view();
            view.handle(event);
            if (help_)
                help_->settle(event, view, frame_.size());
        }
    }
    return true;
}

void Application::advance(std::chrono::milliseconds elapsed) {
    now_ += elapsed;
    screens::View& view = controller_->view();
    view.advance(now_);
    controller_->advance(now_);
    if (help_)
        help_->advance(now_, view, frame_.size());
}

void Application::render() {
    frame_.fill(graphics::Color565{});
    const graphics::Canvas canvas{frame_};
    controller_->view().paint(canvas);
    if (help_)
        help_->paint(canvas);
    window_->present(frame_);
    update_cursor();
}

bool Application::take_transition() {
    auto transition = controller_->take_transition();
    if (!transition)
        return true;
    if (std::holds_alternative<transitions::Quit>(*transition))
        return false;
    if (auto* next = std::get_if<transitions::Switch>(&*transition); next != nullptr && next->next != nullptr) {
        controller_ = std::move(next->next);
        if (help_)
            help_->close();
    }
    return true;
}

void Application::update_cursor() {
    cursors_->advance(now_);
    const platform::CursorImage* wanted = cursors_->image(controller_->view().cursor_state());
    if (wanted == shown_cursor_)
        return;
    window_->set_cursor(wanted);
    shown_cursor_ = wanted;
}

}
