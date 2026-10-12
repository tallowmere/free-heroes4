include(FetchContent)

find_package(Threads REQUIRED)

if(IOS)
  add_library(ZLIB::ZLIB INTERFACE IMPORTED)
  set_target_properties(ZLIB::ZLIB PROPERTIES INTERFACE_LINK_LIBRARIES z)

  set(XZ_TOOL_XZ        OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_XZDEC     OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_LZMADEC   OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_LZMAINFO  OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_SCRIPTS   OFF CACHE BOOL "" FORCE)
  set(XZ_DOC            OFF CACHE BOOL "" FORCE)
  set(XZ_NLS            OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(xz
    GIT_REPOSITORY https://github.com/tukaani-project/xz.git
    GIT_TAG        v5.8.4
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
    SYSTEM)
  FetchContent_MakeAvailable(xz)
  target_include_directories(liblzma SYSTEM INTERFACE
    $<BUILD_INTERFACE:${xz_SOURCE_DIR}/src/liblzma/api>)
  add_library(LibLZMA::LibLZMA ALIAS liblzma)
else()
  find_package(ZLIB REQUIRED)
  find_package(LibLZMA REQUIRED)
endif()

FetchContent_Declare(nlohmann_json
  URL      https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(nlohmann_json)

if(H4_BUILD_PLATFORM)
  if(IOS)
    set(SDL_SHARED        OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC        ON  CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY  OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS         OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES      OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG        release-3.4.14
      GIT_SHALLOW    TRUE
      EXCLUDE_FROM_ALL
      SYSTEM)
    FetchContent_MakeAvailable(SDL3)
  else()
    find_package(SDL3 CONFIG QUIET)
    if(NOT SDL3_FOUND)
      message(WARNING "SDL3 not found -- the platform backend and the game "
                      "executable are disabled (H4_BUILD_PLATFORM=OFF)")
      set(H4_BUILD_PLATFORM OFF CACHE BOOL "" FORCE)
    endif()
  endif()
endif()
