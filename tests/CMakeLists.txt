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

add_executable(test_usb_discovery test_usb_discovery.cpp)
target_link_libraries(test_usb_discovery PRIVATE florid)
target_compile_features(test_usb_discovery PUBLIC cxx_std_20)
add_test(NAME usb_discovery COMMAND test_usb_discovery)

add_test(NAME wirelink_snapshot
    COMMAND "${CMAKE_COMMAND}"
        "-DLF_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DLF_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/snapshot-consumer"
        "-DLF_C_COMPILER=${CMAKE_C_COMPILER}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/wirelink_snapshot.cmake")

# Transport shutdown must remain bounded, including backend-mismatch failures.
set_tests_properties(transport_pipeline wirelink_executor fci_wirelink_endpoint
    usb_discovery PROPERTIES TIMEOUT 30)
