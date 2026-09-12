cmake_minimum_required(VERSION 3.20)

function(add_florid_example s_name s_src)
    add_executable(florid_example_${s_name} ${s_src})
    target_link_libraries(florid_example_${s_name} PRIVATE florid)
    target_compile_features(florid_example_${s_name} PRIVATE cxx_std_20)
endfunction()

add_florid_example(underwater_teleop      underwater_teleop.cpp)
add_florid_example(underwater_control     underwater_control.cpp)
