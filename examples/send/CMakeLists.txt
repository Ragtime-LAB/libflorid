add_executable(willow_example_send main.cpp)
target_compile_features(willow_example_send PUBLIC cxx_std_20)
target_link_libraries(willow_example_send PUBLIC asio ${PROJECT_NAME})