cmake_minimum_required(VERSION 3.20)

project(florid VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS    "Build unit tests (protocol + libflorid)" OFF)
option(BUILD_EXAMPLES  "Build Florid SDK Examples" ON)
option(BUILD_PYFLORID  "Build Python bindings via pybind11" OFF)

include(SDKDependencies)

if(BUILD_TESTS)
    enable_testing()
    include(FindGoogleTest)
    include(GoogleTest)
    add_subdirectory(tests)
endif()

add_subdirectory(protocols protocols)
add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_PYFLORID)
    include(FetchContent)
    find_package(pybind11 QUIET)
    if(NOT pybind11_FOUND)
        FetchContent_Declare(pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.13.6
        )
        FetchContent_MakeAvailable(pybind11)
    endif()
    add_subdirectory(pyflorid)
endif()
