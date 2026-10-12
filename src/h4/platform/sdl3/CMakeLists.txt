h4_module(platform_sdl3
  DEPENDS core platform
  PRIVATE_DEPENDS graphics
  PRIVATE_LIBS SDL3::SDL3
  SOURCES
    sdl_cursors.cpp
    sdl_cursors.hpp
    sdl_dialogs.cpp
    sdl_dialogs.hpp
    sdl_handle.hpp
    sdl_inbox.hpp
    sdl_paths.cpp
    sdl_paths.hpp
    sdl_web_sign_in.cpp
    sdl_web_sign_in.hpp
    sdl_window.cpp
    sdl_window.hpp
)

if(APPLE)
  target_sources(h4_platform_sdl3 PRIVATE sdl_web_sign_in_apple.mm)
  set_source_files_properties(sdl_web_sign_in_apple.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  target_link_libraries(h4_platform_sdl3 PRIVATE "-framework WebKit")
  if(NOT IOS)
    target_link_libraries(h4_platform_sdl3 PRIVATE "-framework AppKit")
  endif()
endif()

if(IOS)
  target_sources(h4_platform_sdl3 PRIVATE sdl_dialogs_uikit.mm sdl_uikit.hpp)
  set_source_files_properties(sdl_dialogs_uikit.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  target_link_libraries(h4_platform_sdl3 PRIVATE "-framework UIKit"
                                                 "-framework UniformTypeIdentifiers")
endif()
