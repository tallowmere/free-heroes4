add_library(h4_policy INTERFACE)
add_library(h4::policy ALIAS h4_policy)

target_compile_options(h4_policy INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual
    -Wnull-dereference -Wimplicit-fallthrough>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

if(APPLE)
  target_link_options(h4_policy INTERFACE -Wl,-no_warn_duplicate_libraries)
endif()

if(H4_WARNINGS_AS_ERRORS)
  target_compile_options(h4_policy INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/WX>)
endif()

if(H4_SANITIZE)
  target_compile_options(h4_policy INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer
    -fno-sanitize-recover=undefined)
  target_link_options(h4_policy INTERFACE -fsanitize=address,undefined)
endif()

if(H4_TSAN)
  if(H4_SANITIZE)
    message(FATAL_ERROR "H4_TSAN and H4_SANITIZE are exclusive")
  endif()
  target_compile_options(h4_policy INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(h4_policy INTERFACE -fsanitize=thread)
endif()
