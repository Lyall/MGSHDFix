cmake_minimum_required(VERSION 3.0)

project(inipp)

add_library(inipp INTERFACE)
add_library(inipp::inipp ALIAS inipp)

target_include_directories(inipp SYSTEM INTERFACE inipp)

target_compile_features(inipp INTERFACE cxx_std_11)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_subdirectory(unittest)
endif()
