add_library(florid
    tick_std.cpp
    timestamp_std.cpp
)

target_link_libraries(florid
    PUBLIC
        fci_protocol
        readerwriterqueue
)

target_include_directories(florid
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
)

target_compile_features(florid PUBLIC cxx_std_20)
