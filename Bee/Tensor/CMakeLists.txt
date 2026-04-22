set(tensor_library_type "STATIC")
set(tensor_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# CUDA 后端自动探测：若 Bee::CUDA target 已定义，则默认启用
# ---------------------------------------------------------------------------
if(TARGET Bee::CUDA)
    option(BEE_TENSOR_WITH_CUDA "启用 Bee::Tensor CUDA 后端" ON)
else()
    option(BEE_TENSOR_WITH_CUDA "启用 Bee::Tensor CUDA 后端" OFF)
endif()

file(GLOB_RECURSE tensor_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE tensor_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

# KernelDispatch.cpp 单源多次编译（每个 ISA 一次），从主库源中剔除
set(tensor_dispatch_source "${CMAKE_CURRENT_SOURCE_DIR}/Cpu/Dispatch/KernelDispatch.cpp")
list(REMOVE_ITEM tensor_sources "${tensor_dispatch_source}")

bee_add_component(
    NAME Tensor
    TYPE "${tensor_library_type}"
    SOURCES
        ${tensor_sources}
    PUBLIC_HEADERS
        ${tensor_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${tensor_private_include_dir}"
    DEPENDS
        Bee::Base
        Bee::SIMD
)

# ---------------------------------------------------------------------------
# 运行期 ISA 分派：为每个本机支持的 ISA 编译一份 KernelDispatch.cpp 为 OBJECT 库
# 对象直接链接进 Tensor；运行期由 simd::current_isa() 选择命名空间
# ---------------------------------------------------------------------------

function(_bee_tensor_add_dispatch_isa isa_upper flags_var)
    set(_obj "TensorDispatch_${isa_upper}")
    add_library(${_obj} OBJECT "${tensor_dispatch_source}")
    set_target_properties(${_obj} PROPERTIES FOLDER "Bee/Dispatch")
    target_compile_features(${_obj} PRIVATE cxx_std_23)
    target_link_libraries(${_obj} PRIVATE Bee::Base Bee::SIMD)
    target_include_directories(${_obj} PRIVATE "${tensor_private_include_dir}")
    target_compile_definitions(${_obj} PRIVATE
        BEE_DISPATCH_ISA_${isa_upper}
        BEE_SIMD_ENABLE_${isa_upper}
    )
    if(${flags_var})
        target_compile_options(${_obj} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${${flags_var}}>
        )
    endif()
    target_link_libraries(Tensor PRIVATE ${_obj})
    # 向 Tensor 暴露“本机具备此 ISA”的宏，供 Ops 层条件编译 switch case
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_HAVE_${isa_upper})
endfunction()

# Scalar 始终构建
add_library(TensorDispatch_SCALAR OBJECT "${tensor_dispatch_source}")
set_target_properties(TensorDispatch_SCALAR PROPERTIES FOLDER "Bee/Dispatch")
target_compile_features(TensorDispatch_SCALAR PRIVATE cxx_std_23)
target_link_libraries(TensorDispatch_SCALAR PRIVATE Bee::Base Bee::SIMD)
target_include_directories(TensorDispatch_SCALAR PRIVATE "${tensor_private_include_dir}")
target_compile_definitions(TensorDispatch_SCALAR PRIVATE BEE_DISPATCH_ISA_SCALAR)
target_link_libraries(Tensor PRIVATE TensorDispatch_SCALAR)
target_compile_definitions(Tensor PUBLIC BEE_TENSOR_HAVE_SCALAR)

if(BEE_SIMD_HAS_SSE2)
    _bee_tensor_add_dispatch_isa(SSE2 BEE_SIMD_SSE2_FLAGS)
endif()
if(BEE_SIMD_HAS_AVX2)
    _bee_tensor_add_dispatch_isa(AVX2 BEE_SIMD_AVX2_FLAGS)
endif()
if(BEE_SIMD_HAS_AVX512)
    _bee_tensor_add_dispatch_isa(AVX512 BEE_SIMD_AVX512_FLAGS)
endif()

set(_bee_tensor_isa_list "Scalar")
if(BEE_SIMD_HAS_SSE2)
    list(APPEND _bee_tensor_isa_list "SSE2")
endif()
if(BEE_SIMD_HAS_AVX2)
    list(APPEND _bee_tensor_isa_list "AVX2")
endif()
if(BEE_SIMD_HAS_AVX512)
    list(APPEND _bee_tensor_isa_list "AVX512")
endif()
list(JOIN _bee_tensor_isa_list " " _bee_tensor_isa_str)
message(STATUS "Bee::Tensor 运行期 ISA 分派: ${_bee_tensor_isa_str}")

# 若启用 CUDA 后端：链接 Bee::CUDA 并定义 BEE_TENSOR_WITH_CUDA
if(BEE_TENSOR_WITH_CUDA)
    if(NOT TARGET Bee::CUDA)
        message(FATAL_ERROR "BEE_TENSOR_WITH_CUDA=ON 但 Bee::CUDA target 不存在")
    endif()
    message(STATUS "Bee::Tensor CUDA 后端: ON (link Bee::CUDA)")
    target_link_libraries(Tensor PRIVATE Bee::CUDA)
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_WITH_CUDA)
else()
    message(STATUS "Bee::Tensor CUDA 后端: OFF")
endif()
