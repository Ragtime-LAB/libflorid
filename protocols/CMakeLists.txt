cmake_minimum_required(VERSION 3.21)
project(Florid_Protocols)

set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers")
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/florid/protocols")

add_library(florid_protocol INTERFACE)
target_include_directories(florid_protocol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
