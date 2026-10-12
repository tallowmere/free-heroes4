h4_executable(openh4
  SOURCES main.cpp first_boot.cpp first_boot.hpp app_icon.hpp
  LINK h4::app h4::assets h4::cursor h4::io h4::model h4::net h4::platform_sdl3 h4::screens
       SDL3::SDL3)
if(APPLE)
  target_link_libraries(openh4 PRIVATE h4::net_apple)
endif()

set(icon_png ${CMAKE_CURRENT_SOURCE_DIR}/icon/openh4-256.png)
file(READ ${icon_png} icon_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," H4_APP_ICON_BYTES "${icon_hex}")
configure_file(app_icon.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/app_icon.cpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${icon_png})
target_sources(openh4 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.cpp)
target_include_directories(openh4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(IOS OR CMAKE_GENERATOR STREQUAL "Xcode")
  include(H4AppleBundle)
  h4_apple_bundle(openh4)
endif()
