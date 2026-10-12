h4_module(assets
  DEPENDS core formats graphics
  PRIVATE_DEPENDS io
  SOURCES
    asset_library.cpp
    asset_decode.hpp
    asset_library.hpp
    asset_prefetch.cpp
    assets.hpp
    prefetched.hpp
    request.hpp
    required.cpp
    required.hpp
)
