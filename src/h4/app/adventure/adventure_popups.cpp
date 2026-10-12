#include <string>
#include <utility>
#include <vector>

#include "h4/app/adventure/adventure_controller.hpp"
#include "h4/app/adventure/system_menu.hpp"
#include "h4/app/boot/scroll_menu.hpp"
#include "h4/assets/assets.hpp"
#include "h4/screens/boot/scroll_menu_view.hpp"
#include "h4/screens/dialogs/basic_dialog.hpp"

namespace h4::app {

void AdventureController::notice_over(const rules::PathField::PathStep& step, screens::BasicDialogContent content) {
    messages_.add(std::move(content), screens::DialogPlacement::standing_on(above_object(step)));
    messages_.show();
}

void AdventureController::on_messages_read() {
    if (chest_visit_ && !chest_.is_open())
        open_chest(*chest_visit_);
}

void AdventureController::open_system_menu() {
    const auto scroll = open_scroll_menu(system_scroll_,
                                         *assets_,
                                         view_->frame_size(),
                                         view_->button_rect(screens::AdventureButton::system_menu).origin(),
                                         text_keys(std::span<const SystemMenuEntry>{system_menu_}),
                                         [this](std::size_t index) { on_system_menu(index); });
    if (report_.failed(scroll))
        return;
}

void AdventureController::on_system_menu(std::size_t index) {
    system_scroll_.close();
    if (index >= system_menu_.size())
        return;
    switch (system_menu_[index].choice) {
    case SystemMenuChoice::quit: request(transitions::Quit{}); break;
    case SystemMenuChoice::game_settings: open_game_settings(); break;
    case SystemMenuChoice::main_menu:
        if (menu_)
            if (auto next = menu_(settings_))
                request(transitions::Switch{std::move(next)});
        break;
    default: report_.not_ported(std::string{display_name(system_menu_[index].choice)}); break;
    }
}

void AdventureController::open_game_settings() {
    const auto opened = settings_popup_.open(*assets_,
                                             view_->frame_size(),
                                             settings_,
                                             {report_,
                                              [this](const model::GameSettings& chosen) {
                                                  set_settings(chosen);
                                                  if (saver_)
                                                      saver_(chosen);
                                              },
                                              {}});
    if (report_.failed(opened))
        return;
}

void AdventureController::on_key(platform::Key key) {
    if (settings_popup_.on_key(key) || question_.on_key(key) || army_window_.on_key(key) || chest_.on_key(key) ||
        messages_.on_key(key) || system_scroll_.on_key(key))
        return;
}

}
