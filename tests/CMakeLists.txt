add_executable(test_transport_pipeline test_transport_pipeline.cpp)

target_link_libraries(test_transport_pipeline
    PRIVATE
        florid
        asio_standalone
)

target_compile_features(test_transport_pipeline PUBLIC cxx_std_20)

add_test(NAME transport_pipeline COMMAND test_transport_pipeline)

if(TARGET florid_wirelink_host)
    add_executable(test_wirelink_executor test_wirelink_executor.cpp)
    target_link_libraries(test_wirelink_executor PRIVATE florid::wirelink_host)
    target_compile_features(test_wirelink_executor PUBLIC cxx_std_20)
    add_test(NAME wirelink_executor COMMAND test_wirelink_executor)
endif()
