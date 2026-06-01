add_library(${PROJECT_NAME}
        Arm.cpp
        core/ActiveControl.cpp
        core/ArmCore.cpp
        detail/TcpClient.cpp
        detail/UdpClient.cpp
        detail/tick_std.cpp
        detail/timestamp_std.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/3rdparty/BasicLinearAlgebra)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC florid_protocol asio::asio)
