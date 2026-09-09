cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(florid_acados_runtime LANGUAGES C)
endif()

# Deliberately avoid upstream's global flags, install rules and source-tree
# generated headers. This is libflorid's pinned, double-precision C runtime.
include(sources.cmake)
set(BLASFEO_TARGET GENERIC CACHE STRING "Vendored BLASFEO backend (GENERIC is portable)")
set_property(CACHE BLASFEO_TARGET PROPERTY STRINGS ${LF_BLASFEO_BACKENDS})
set(HPIPM_TARGET GENERIC CACHE STRING "Vendored HPIPM backend (GENERIC only)")
set_property(CACHE HPIPM_TARGET PROPERTY STRINGS GENERIC)
if(NOT BLASFEO_TARGET IN_LIST LF_BLASFEO_BACKENDS)
    message(FATAL_ERROR "Unsupported vendored BLASFEO_TARGET=${BLASFEO_TARGET}; choose ${LF_BLASFEO_BACKENDS}")
endif()
if(NOT HPIPM_TARGET STREQUAL "GENERIC")
    message(FATAL_ERROR "The vendored runtime requires HPIPM_TARGET=GENERIC")
endif()
foreach(extra IN ITEMS HPMPC OOQP QORE QPOASES DAQP QPDUNES OSQP CLARABEL OPENMP SYSTEM_BLASFEO)
    if(ACADOS_WITH_${extra})
        message(FATAL_ERROR "ACADOS_WITH_${extra} is not part of libflorid's vendored runtime")
    endif()
endforeach()

set(TARGET "${BLASFEO_TARGET}")
set(TARGET2 PHANTOM)
set(LA HIGH_PERFORMANCE)
set(MF PANELMAJ)
set(EXT_DEP ON)
set(EXT_DEP_MALLOC ON)
set(BLAS_API OFF)
set(FORTRAN_BLAS_API OFF)
foreach(feature IN ITEMS AVX2 FMA SSE3 AVX VFPv3 NEON VFPv4 NEONv2)
    set(TARGET_NEED_FEATURE_${feature} OFF)
endforeach()
set(lf_arch_flags)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" lf_processor)
if(NOT BLASFEO_TARGET STREQUAL "GENERIC")
    if(MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
        message(FATAL_ERROR "Use BLASFEO_TARGET=GENERIC with MSVC/clang-cl")
    endif()
    if(BLASFEO_TARGET MATCHES "^X64_")
        if(NOT lf_processor MATCHES "^(x86_64|amd64|x64)$" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR "${BLASFEO_TARGET} requires an x86-64 toolchain")
        endif()
        if(BLASFEO_TARGET STREQUAL "X64_INTEL_HASWELL")
            set(lf_arch_flags -m64 -mavx -mavx2 -mfma)
            set(TARGET_NEED_FEATURE_AVX2 1)
            set(TARGET_NEED_FEATURE_FMA 1)
        else()
            set(lf_arch_flags -m64 -msse3)
            set(TARGET_NEED_FEATURE_SSE3 1)
        endif()
    else()
        if(NOT lf_processor MATCHES "^(aarch64|arm64)$" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR "${BLASFEO_TARGET} requires an ARM64 toolchain")
        endif()
        set(lf_arch_flags -march=armv8-a+crc+crypto+simd)
        set(TARGET_NEED_FEATURE_VFPv4 1)
        set(TARGET_NEED_FEATURE_NEONv2 1)
        if(BLASFEO_TARGET STREQUAL "ARMV8A_APPLE_M1")
            set(TARGET2 ARMV8A_ARM_CORTEX_A57)
        endif()
    endif()
    # No try_run: target selection also works when cross-compiling.
    enable_language(ASM)
endif()

configure_file(external/blasfeo/blasfeo_target.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/include/blasfeo_target.h" @ONLY)

add_library(lf_blasfeo STATIC ${LF_BLASFEO_${BLASFEO_TARGET}_SOURCES})
target_include_directories(lf_blasfeo PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/blasfeo/include")
target_compile_definitions(lf_blasfeo PRIVATE
    TARGET_${TARGET} TARGET_${TARGET2} LA_HIGH_PERFORMANCE MF_PANELMAJ
    BLASFEO_REF_API DP_ROUTINES K_MAX_STACK=300 USE_C99_MATH
    EXT_DEP EXT_DEP_MALLOC EXTERNAL_BLAS_NONE
    "$<$<COMPILE_LANGUAGE:ASM>:MACRO_LEVEL=1>")
foreach(flag IN LISTS lf_arch_flags)
    target_compile_options(lf_blasfeo PRIVATE "$<$<COMPILE_LANGUAGE:C>:${flag}>")
endforeach()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(lf_blasfeo PRIVATE OS_LINUX)
elseif(APPLE)
    target_compile_definitions(lf_blasfeo PRIVATE OS_MAC)
elseif(WIN32 OR CYGWIN)
    target_compile_definitions(lf_blasfeo PRIVATE OS_WINDOWS)
endif()
if(NOT MSVC)
    target_link_libraries(lf_blasfeo PUBLIC m)
endif()

add_library(lf_hpipm STATIC ${LF_HPIPM_SOURCES})
target_include_directories(lf_hpipm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/hpipm/include")
target_compile_definitions(lf_hpipm PRIVATE TARGET_GENERIC USE_C99_MATH RUNTIME_CHECKS)
target_link_libraries(lf_hpipm PUBLIC lf_blasfeo)

add_library(lf_acados STATIC ${LF_ACADOS_SOURCES})
add_library(florid::acados ALIAS lf_acados)
target_include_directories(lf_acados PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces"
    "${CMAKE_CURRENT_SOURCE_DIR}/external")
target_compile_definitions(lf_acados PRIVATE "$<$<CONFIG:Debug>:DEBUG>")
if(WIN32 OR CYGWIN)
    # Public timing headers include Windows.h. Keep min/max and legacy
    # Winsock macros out of both generated traits and downstream SDK code.
    target_compile_definitions(lf_acados PUBLIC STATIC_LINKED NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
target_link_libraries(lf_acados PUBLIC lf_hpipm)

foreach(runtime IN ITEMS lf_blasfeo lf_hpipm lf_acados)
    set_target_properties(${runtime} PROPERTIES
        C_STANDARD 99 C_STANDARD_REQUIRED YES POSITION_INDEPENDENT_CODE YES)
endforeach()
message(STATUS "libflorid vendored acados: BLASFEO=${BLASFEO_TARGET}, HPIPM=GENERIC, double precision")
