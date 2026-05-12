add_executable(florid_protocol_test
    test_version.cpp
    test_packets.cpp
    test_dispatcher.cpp
    test_parser.cpp
)

target_link_libraries(florid_protocol_test PRIVATE florid_protocol GTest::gtest_main)
target_compile_features(florid_protocol_test PRIVATE cxx_std_17)

gtest_discover_tests(florid_protocol_test)
