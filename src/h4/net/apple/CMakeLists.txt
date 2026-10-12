h4_module(net_apple
  DEPENDS core net
  SOURCES
    apple_http.hpp
    apple_http.mm
)
set_source_files_properties(apple_http.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
target_link_libraries(h4_net_apple PRIVATE "-framework Foundation")
