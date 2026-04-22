include(CheckLanguage)

check_language(CUDA)
if (NOT CMAKE_CUDA_COMPILER)
    message(WARNING "CUDA compiler not found. Skipping Bee::CUDA component.")
    return()
endif ()

enable_language(CUDA)

# 单一目标架构：sm_120（Blackwell, RTX 50 系列）。plan-cuda §4 规定。
# 以 CACHE 方式发布，使 Tests/CUDA/ 等同级目录同样可见。
set(CMAKE_CUDA_ARCHITECTURES "120" CACHE STRING "Bee::CUDA target architectures" FORCE)

find_package(CUDAToolkit QUIET)
if (NOT CUDAToolkit_FOUND)
    message(WARNING "CUDA Toolkit not found. Skipping Bee::CUDA component.")
    return()
endif ()

if (NOT DEFINED BEE_MIN_CUDA_VERSION)
    set(BEE_MIN_CUDA_VERSION "13.0")
endif ()

if (CUDAToolkit_VERSION VERSION_LESS BEE_MIN_CUDA_VERSION)
    message(WARNING "CUDA Toolkit ${CUDAToolkit_VERSION} 过旧，要求 >= ${BEE_MIN_CUDA_VERSION}。跳过 Bee::CUDA 组件。")
    return()
endif ()

# ── CUTLASS 探测 ────────────────────────────────────────────────────────────
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

message(STATUS "Found CUDA Toolkit: ${CUDAToolkit_VERSION}")
if (BEE_HAS_CUTLASS)
    message(STATUS "Found CUTLASS include: ${BEE_CUTLASS_INCLUDE_DIR}")
else ()
    message(STATUS "CUTLASS not found. Bee::CUDA 将不构建依赖 CUTLASS 的 kernel。")
endif ()

# ── 源文件与 target ────────────────────────────────────────────────────────
set(cuda_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE cuda_public_headers CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cuh"
)
file(GLOB_RECURSE cuda_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

add_library(CUDA STATIC
        ${cuda_sources}
        ${cuda_public_headers}
)

target_include_directories(CUDA
        PUBLIC
        $<BUILD_INTERFACE:${BEE_PUBLIC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        "${cuda_private_include_dir}"
)

if (BEE_HAS_CUTLASS)
    target_include_directories(CUDA PRIVATE ${BEE_CUTLASS_INCLUDE_DIR})
endif ()

target_link_libraries(CUDA
        PUBLIC
        Bee::Base
        CUDA::cudart
)

if (TARGET cutlass::cutlass)
    target_link_libraries(CUDA PRIVATE cutlass::cutlass)
endif ()

target_compile_features(CUDA
        PUBLIC
        cxx_std_23
        PRIVATE
        cuda_std_20
)

set_target_properties(CUDA PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
)

if (NOT CUDAToolkit_VERSION)
    set(CUDAToolkit_VERSION "unknown")
endif ()

target_compile_definitions(CUDA
        PUBLIC
        BEE_CUDA_VERSION_STRING="${CUDAToolkit_VERSION}"
        BEE_HAS_CUTLASS=$<BOOL:${BEE_HAS_CUTLASS}>
)

set_target_properties(CUDA PROPERTIES
        CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
        CUDA_SEPARABLE_COMPILATION OFF
        FOLDER "Bee/Libraries"
)

set(cuda_all_sources ${cuda_sources} ${cuda_public_headers})
bee_group_sources(
        TARGET CUDA
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES ${cuda_all_sources}
)

add_library(Bee::CUDA ALIAS CUDA)
