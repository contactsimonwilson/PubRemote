# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.* ${CMAKE_SOURCE_DIR}/src/remote/*.*  ${CMAKE_SOURCE_DIR}/src/ui/*.*)

idf_component_register(SRCS ${app_sources})
