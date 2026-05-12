cmake_minimum_required(VERSION 3.21)
project(Florid_Protocols)

option(BUILD_PROTOCOL_TESTS "Build Florid Protocol Examples" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers")
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/florid/protocols")

add_library(florid_protocol INTERFACE)
target_include_directories(florid_protocol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (BUILD_PROTOCOL_TESTS)
  include(FindGoogleTest)
  enable_testing()
  include(GoogleTest)
  add_subdirectory(tests)
endif ()
