cmake_minimum_required(VERSION 3.20)

project(florid VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_EXAMPLES "Build Florid SDK Examples" ON)

include(SDKDependencies)

add_subdirectory(protocols protocols)
add_subdirectory(src)

if(BUILD_EXAMPLES)

  add_subdirectory(examples)

endif()



