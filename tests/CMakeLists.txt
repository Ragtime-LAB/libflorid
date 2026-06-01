add_executable(florid_test
    test_model.cpp
    test_active_control.cpp
)

target_include_directories(florid_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/BasicLinearAlgebra
)

target_compile_features(florid_test PRIVATE cxx_std_17)

target_link_libraries(florid_test PRIVATE
    ${PROJECT_NAME}
    GTest::gtest_main
)

gtest_discover_tests(florid_test)
