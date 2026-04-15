include(CheckLanguage)

check_language(CUDA)
if (NOT CMAKE_CUDA_COMPILER)
    message(WARNING "CUDA compiler not found. Skipping cuKernels component.")
    return()
endif ()

enable_language(CUDA)

set(CMAKE_CUDA_ARCHITECTURES "89;120") # 89;90;
set(CMAKE_CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}" PARENT_SCOPE)

find_package(CUDAToolkit QUIET)
if (NOT CUDAToolkit_FOUND)
    message(WARNING "CUDA Toolkit not found. Skipping cuKernels component.")
    return()
endif ()

if (NOT DEFINED BEE_MIN_CUDA_VERSION)
    set(BEE_MIN_CUDA_VERSION "12.0")
endif ()

if (CUDAToolkit_VERSION VERSION_LESS BEE_MIN_CUDA_VERSION)
    message(WARNING "CUDA Toolkit ${CUDAToolkit_VERSION} is too old. Required >= ${BEE_MIN_CUDA_VERSION}.")
    return()
endif ()

set(_cutlass_hints)
if (DEFINED CUTLASS_ROOT)
    list(APPEND _cutlass_hints "${CUTLASS_ROOT}")
endif ()
if (DEFINED ENV{CUTLASS_ROOT})
    list(APPEND _cutlass_hints "$ENV{CUTLASS_ROOT}")
endif ()
if (DEFINED ENV{CUTLASS_PATH})
    list(APPEND _cutlass_hints "$ENV{CUTLASS_PATH}")
endif ()
if (DEFINED ENV{CUTLASS_DIR})
    list(APPEND _cutlass_hints "$ENV{CUTLASS_DIR}")
endif ()
if (DEFINED ENV{ENV_HOME})
    list(APPEND _cutlass_hints "$ENV{ENV_HOME}/NVIDIA/cutlass")
endif ()

find_path(BEE_CUTLASS_INCLUDE_DIR
        NAMES cutlass/cutlass.h
        HINTS ${_cutlass_hints}
        PATH_SUFFIXES include
)

set(BEE_HAS_CUTLASS OFF)
if (BEE_CUTLASS_INCLUDE_DIR)
    set(BEE_HAS_CUTLASS ON)
endif ()

set(_has_sm90 FALSE)
if (DEFINED CMAKE_CUDA_ARCHITECTURES)
    foreach (arch IN LISTS CMAKE_CUDA_ARCHITECTURES)
        string(REGEX REPLACE "[^0-9]" "" arch_num "${arch}")
        if (arch_num AND arch_num GREATER_EQUAL 90)
            set(_has_sm90 TRUE)
        endif ()
    endforeach ()
endif ()

set(BEE_HAS_WGMMA OFF)
if (BEE_HAS_CUTLASS
        AND CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0"
        AND _has_sm90
        AND EXISTS "${BEE_CUTLASS_INCLUDE_DIR}/cute/arch/mma_sm90.hpp")
    set(BEE_HAS_WGMMA ON)
endif ()

message(STATUS "Found CUDA Toolkit: ${CUDAToolkit_VERSION}")
if (BEE_HAS_CUTLASS)
    message(STATUS "Found CUTLASS include: ${BEE_CUTLASS_INCLUDE_DIR}")
else ()
    message(STATUS "CUTLASS not found. Building without CUTLASS-dependent kernels.")
endif ()

if (BEE_HAS_WGMMA)
    message(STATUS "WGMMA available (nvcc=${CMAKE_CUDA_COMPILER_VERSION}, arch=${CMAKE_CUDA_ARCHITECTURES})")
else ()
    message(STATUS "WGMMA not available. Requirements: CUTLASS + nvcc>=12.0 + sm90 + cute/arch/mma_sm90.hpp")
endif ()

set(cuda_kernel_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE cuda_kernel_public_headers CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cuh"
)
file(GLOB_RECURSE cuda_kernel_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

add_library(cuKernels STATIC
        ${cuda_kernel_sources}
        ${cuda_kernel_public_headers}
)

target_include_directories(cuKernels
        PUBLIC
        $<BUILD_INTERFACE:${BEE_PUBLIC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        "${cuda_kernel_private_include_dir}"
)

if (BEE_HAS_CUTLASS)
    target_include_directories(cuKernels PRIVATE ${BEE_CUTLASS_INCLUDE_DIR})
endif ()

target_link_libraries(cuKernels
        PUBLIC
        Bee::Base
        CUDA::cudart
)

if (TARGET cutlass::cutlass)
    target_link_libraries(cuKernels PRIVATE cutlass::cutlass)
endif ()

target_compile_features(cuKernels
        PUBLIC
        cxx_std_20
        PRIVATE
        cuda_std_20
)

set_target_properties(cuKernels PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
)

if (NOT CUDAToolkit_VERSION)
    set(CUDAToolkit_VERSION "unknown")
endif ()

target_compile_definitions(cuKernels
        PUBLIC
        BEE_CUDA_VERSION_STRING="${CUDAToolkit_VERSION}"
        BEE_HAS_CUTLASS=$<BOOL:${BEE_HAS_CUTLASS}>
        BEE_HAS_WGMMA=$<BOOL:${BEE_HAS_WGMMA}>
)

set_target_properties(cuKernels PROPERTIES
        CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
        CUDA_SEPARABLE_COMPILATION OFF
        FOLDER "Bee"
)

set(cuda_kernel_all_sources ${cuda_kernel_sources} ${cuda_kernel_public_headers})
bee_group_sources(
        TARGET cuKernels
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES ${cuda_kernel_all_sources}
)

add_library(Bee::cuKernels ALIAS cuKernels)
