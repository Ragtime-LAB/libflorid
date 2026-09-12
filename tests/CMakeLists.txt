add_executable(test_wirelink_executor test_wirelink_executor.cpp)
target_link_libraries(test_wirelink_executor PRIVATE florid::wirelink_host)
target_compile_features(test_wirelink_executor PUBLIC cxx_std_20)
add_test(NAME wirelink_executor COMMAND test_wirelink_executor)

add_executable(test_fci_upgrade_client test_fci_upgrade_client.cpp)
target_link_libraries(test_fci_upgrade_client PRIVATE florid florid::wirelink_host)
target_compile_features(test_fci_upgrade_client PRIVATE cxx_std_20)
target_compile_definitions(test_fci_upgrade_client PRIVATE FCI_TEST_DEVICE)
add_test(NAME fci_upgrade_client COMMAND test_fci_upgrade_client)
set_tests_properties(fci_upgrade_client PROPERTIES TIMEOUT 30)

add_executable(test_underwater_teleop test_underwater_teleop.cpp)
target_link_libraries(test_underwater_teleop PRIVATE florid florid::wirelink_host)
target_compile_definitions(test_underwater_teleop PRIVATE FCI_TEST_DEVICE)
add_test(NAME underwater_teleop COMMAND test_underwater_teleop)
set_tests_properties(underwater_teleop PROPERTIES TIMEOUT 30)

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
set_tests_properties(wirelink_executor usb_discovery PROPERTIES TIMEOUT 30)
