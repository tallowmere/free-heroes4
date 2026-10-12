h4_module(net
  DEPENDS core
  PRIVATE_LIBS nlohmann_json::nlohmann_json
  SOURCES
    gog/gog_api.cpp
    gog/gog_api.hpp
    gog/gog_download.cpp
    gog/gog_download.hpp
    http.hpp
)
