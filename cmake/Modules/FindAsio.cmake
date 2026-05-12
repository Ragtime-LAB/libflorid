set(ASIO_REPOSITORY
        "https://github.com/chriskohlhoff/asio"
)

FetchContent_Declare(
        asio
        GIT_REPOSITORY ${ASIO_REPOSITORY}
        GIT_TAG asio-1-38-0
        GIT_SHALLOW true
)

FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)

target_include_directories(
        asio INTERFACE
        $<BUILD_INTERFACE:${asio_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(WIN32)
    target_link_libraries(asio INTERFACE ws2_32)
endif()