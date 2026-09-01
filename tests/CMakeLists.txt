add_executable(test_transport_pipeline test_transport_pipeline.cpp)

target_link_libraries(test_transport_pipeline
    PRIVATE
        florid
        florid::wirelink_host
        asio_standalone
)

target_compile_features(test_transport_pipeline PUBLIC cxx_std_20)

add_test(NAME transport_pipeline COMMAND test_transport_pipeline)

add_executable(test_wirelink_executor test_wirelink_executor.cpp)
target_link_libraries(test_wirelink_executor PRIVATE florid::wirelink_host)
target_compile_features(test_wirelink_executor PUBLIC cxx_std_20)
add_test(NAME wirelink_executor COMMAND test_wirelink_executor)

add_executable(test_fci_wirelink_endpoint test_fci_wirelink_endpoint.cpp)
target_link_libraries(test_fci_wirelink_endpoint PRIVATE florid::wirelink_host)
target_compile_features(test_fci_wirelink_endpoint PUBLIC cxx_std_20)
add_test(NAME fci_wirelink_endpoint COMMAND test_fci_wirelink_endpoint)
