function(h4_module name)
  cmake_parse_arguments(PARSE_ARGV 1 ARG ""  ""
                        "SOURCES;DEPENDS;PRIVATE_DEPENDS;LIBS;PRIVATE_LIBS")
  set(target h4_${name})
  add_library(${target} STATIC ${ARG_SOURCES})
  add_library(h4::${name} ALIAS ${target})
  target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/src)

  list(TRANSFORM ARG_DEPENDS         PREPEND h4:: OUTPUT_VARIABLE public_deps)
  list(TRANSFORM ARG_PRIVATE_DEPENDS PREPEND h4:: OUTPUT_VARIABLE private_deps)
  target_link_libraries(${target}
    PUBLIC  ${public_deps}  ${ARG_LIBS}
    PRIVATE ${private_deps} ${ARG_PRIVATE_LIBS} h4::policy)
endfunction()

function(h4_executable name)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LINK")
  add_executable(${name} ${ARG_SOURCES})
  target_link_libraries(${name} PRIVATE ${ARG_LINK} h4::policy)
endfunction()
