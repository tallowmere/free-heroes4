h4_module(graphics
  DEPENDS core
  PRIVATE_DEPENDS formats
  SOURCES
    canvas.cpp
    canvas.hpp
    nearest.hpp
    color.hpp
    coverage.cpp
    coverage.hpp
    color565.hpp
    diamond.hpp
    rgba_image.cpp
    rgba_image.hpp
    shade565.cpp
    shade565.hpp
    surface565.cpp
    surface565.hpp
    terrain_blit.cpp
    terrain_blit.hpp
    text.cpp
    text.hpp
    tiled_fill.cpp
    tiled_fill.hpp
)
