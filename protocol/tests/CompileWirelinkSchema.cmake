foreach(_required IN ITEMS
        WLC_EXECUTABLE
        SCHEMA
        PROFILE
        MODULE
        OUTPUT_DIR
        C_COMPILER
        CXX_COMPILER
        WIRELINK_INCLUDE_DIRS)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(_profile_args --profile "${PROFILE}")
foreach(_service IN LISTS SERVICES_PROFILE)
    list(APPEND _profile_args --profile "${_service}")
endforeach()
execute_process(
    COMMAND "${WLC_EXECUTABLE}" compile "${SCHEMA}"
        ${_profile_args}
        --out-dir "${OUTPUT_DIR}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR
        "WLC compile failed (${_result})\n${_stdout}\n${_stderr}")
endif()

foreach(_artifact IN ITEMS
        ${MODULE}.h
        ${MODULE}.c
        ${MODULE}_bindings.h
        ${MODULE}_bindings.c
        ${MODULE}_runtime.h
        ${MODULE}_runtime.c
        ${MODULE}_manifest.json)
    if(NOT EXISTS "${OUTPUT_DIR}/${_artifact}")
        message(FATAL_ERROR
            "WLC did not produce ${OUTPUT_DIR}/${_artifact}")
    endif()
endforeach()

set(_include_dirs "${OUTPUT_DIR}" ${WIRELINK_INCLUDE_DIRS})

if(C_COMPILER_ID MATCHES "GNU|Clang")
    set(_c_strict_flags -std=c11 -Wall -Wextra -Wpedantic -Werror)
    foreach(_include_dir IN LISTS _include_dirs)
        list(APPEND _c_include_args "-I${_include_dir}")
    endforeach()
elseif(C_COMPILER_ID STREQUAL "MSVC")
    set(_c_strict_flags /std:c11 /W4 /WX)
    foreach(_include_dir IN LISTS _include_dirs)
        list(APPEND _c_include_args "/I${_include_dir}")
    endforeach()
else()
    message(FATAL_ERROR
        "strict C11 schema test does not support ${C_COMPILER_ID}")
endif()

foreach(_source IN ITEMS
        ${MODULE}.c
        ${MODULE}_bindings.c
        ${MODULE}_runtime.c)
    if(C_COMPILER_ID STREQUAL "MSVC")
        execute_process(
            COMMAND "${C_COMPILER}" ${_c_strict_flags} ${_c_include_args}
                /c "${OUTPUT_DIR}/${_source}"
                "/Fo${OUTPUT_DIR}/${_source}.obj"
            RESULT_VARIABLE _compile_result
            OUTPUT_VARIABLE _compile_stdout
            ERROR_VARIABLE _compile_stderr)
    else()
        execute_process(
            COMMAND "${C_COMPILER}" ${_c_strict_flags} ${_c_include_args}
                -c "${OUTPUT_DIR}/${_source}"
                -o "${OUTPUT_DIR}/${_source}.o"
            RESULT_VARIABLE _compile_result
            OUTPUT_VARIABLE _compile_stdout
            ERROR_VARIABLE _compile_stderr)
    endif()
    if(NOT _compile_result EQUAL 0)
        message(FATAL_ERROR
            "strict C11 compile failed for ${_source} (${_compile_result})\n"
            "${_compile_stdout}\n${_compile_stderr}")
    endif()
endforeach()

set(_cpp_smoke "${OUTPUT_DIR}/${MODULE}_header_smoke.cpp")
file(WRITE "${_cpp_smoke}"
    "#include \"${MODULE}.h\"\n"
    "#include \"${MODULE}_bindings.h\"\n"
    "#include \"${MODULE}_runtime.h\"\n"
    "int main() { return 0; }\n")

if(CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(_cxx_strict_flags -std=c++20 -Wall -Wextra -Wpedantic -Werror)
    foreach(_include_dir IN LISTS _include_dirs)
        list(APPEND _cxx_include_args "-I${_include_dir}")
    endforeach()
elseif(CXX_COMPILER_ID STREQUAL "MSVC")
    set(_cxx_strict_flags /std:c++20 /W4 /WX)
    foreach(_include_dir IN LISTS _include_dirs)
        list(APPEND _cxx_include_args "/I${_include_dir}")
    endforeach()
else()
    message(FATAL_ERROR
        "strict C++20 schema test does not support ${CXX_COMPILER_ID}")
endif()

if(CXX_COMPILER_ID STREQUAL "MSVC")
    execute_process(
        COMMAND "${CXX_COMPILER}" ${_cxx_strict_flags} ${_cxx_include_args}
            /c "${_cpp_smoke}" "/Fo${_cpp_smoke}.obj"
        RESULT_VARIABLE _cpp_result
        OUTPUT_VARIABLE _cpp_stdout
        ERROR_VARIABLE _cpp_stderr)
else()
    execute_process(
        COMMAND "${CXX_COMPILER}" ${_cxx_strict_flags} ${_cxx_include_args}
            -c "${_cpp_smoke}" -o "${_cpp_smoke}.o"
        RESULT_VARIABLE _cpp_result
        OUTPUT_VARIABLE _cpp_stdout
        ERROR_VARIABLE _cpp_stderr)
endif()
if(NOT _cpp_result EQUAL 0)
    message(FATAL_ERROR
        "strict C++20 header compile failed (${_cpp_result})\n"
        "${_cpp_stdout}\n${_cpp_stderr}")
endif()

message(STATUS "${_stdout}")
