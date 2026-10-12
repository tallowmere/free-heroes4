#include <utility>

#include "h4/app/boot/game_settings_popup.hpp"
#include "h4/app/boot/main_menu_controller.hpp"

namespace h4::app {

void MainMenuController::open_game_settings() {
    const auto opened = settings_popup_.open(*assets_,
                                             view_->frame_size(),
                                             settings_,
                                             {report_,
                                              [this](const model::GameSettings& chosen) {
                                                  set_settings(chosen);
                                                  if (saver_)
                                                      saver_(chosen);
                                              },
                                              [this] { set_actions_enabled(true); }});
    if (report_.failed(opened))
        return;
    set_actions_enabled(false);
}

}
