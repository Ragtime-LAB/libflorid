cmake_minimum_required(VERSION 3.20)

function(add_florid_example s_name)
    add_executable(florid_example_${s_name} ${s_name}.cpp)
    target_link_libraries(florid_example_${s_name} PRIVATE florid)
    target_compile_features(florid_example_${s_name} PRIVATE cxx_std_20)
endfunction()

add_florid_example(echo_arm_state)
add_florid_example(joint_sine_motion)
add_florid_example(gravity_compensation)
