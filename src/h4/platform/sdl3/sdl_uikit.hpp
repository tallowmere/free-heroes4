#pragma once

#include <SDL3/SDL.h>
#import <UIKit/UIKit.h>

namespace h4::platform::sdl3 {

inline UIViewController* top_view_controller(SDL_Window* window) {
    const SDL_PropertiesID properties = SDL_GetWindowProperties(window);
    UIWindow* host = (__bridge UIWindow*)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
    UIViewController* over = host.rootViewController;
    while (over.presentedViewController != nil)
        over = over.presentedViewController;
    return over;
}

}
