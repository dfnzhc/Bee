set(tensor_library_type "STATIC")
set(tensor_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# CUDA 后端占位选项
# ---------------------------------------------------------------------------
option(BEE_TENSOR_WITH_CUDA "启用 Bee::Tensor CUDA 后端（当前为 stub 占位）" OFF)

file(GLOB_RECURSE tensor_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE tensor_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

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
# SIMD 后端选项（由 Bee::SIMD 组件在此之前完成探测并导出 cache 变量）
# ---------------------------------------------------------------------------
set(BEE_TENSOR_SIMD "AUTO" CACHE STRING "Tensor SIMD 后端（AUTO/SCALAR/SSE2/AVX2/AVX512）")
set_property(CACHE BEE_TENSOR_SIMD PROPERTY STRINGS AUTO SCALAR SSE2 AVX2 AVX512)

string(TOUPPER "${BEE_TENSOR_SIMD}" _bee_simd_input)
if(NOT _bee_simd_input MATCHES "^(AUTO|SCALAR|SSE2|AVX2|AVX512)$")
    message(FATAL_ERROR "BEE_TENSOR_SIMD 非法值：'${BEE_TENSOR_SIMD}'，期望 AUTO/SCALAR/SSE2/AVX2/AVX512 之一")
endif()

# 根据用户选择或 AUTO 探测确定最终 ISA
if(_bee_simd_input STREQUAL "AUTO")
    # 按最高优先级自动选择
    if(BEE_SIMD_HAS_AVX512)
        set(BEE_TENSOR_SIMD_RESOLVED "AVX512")
    elseif(BEE_SIMD_HAS_AVX2)
        set(BEE_TENSOR_SIMD_RESOLVED "AVX2")
    elseif(BEE_SIMD_HAS_SSE2)
        set(BEE_TENSOR_SIMD_RESOLVED "SSE2")
    else()
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    endif()
else()
    set(BEE_TENSOR_SIMD_RESOLVED "${_bee_simd_input}")
    # 验证用户指定的 ISA 确实被本机支持
    if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX512" AND NOT BEE_SIMD_HAS_AVX512)
        message(WARNING "BEE_TENSOR_SIMD=AVX512 但本机不支持 AVX-512，已降级为 SCALAR")
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX2" AND NOT BEE_SIMD_HAS_AVX2)
        message(WARNING "BEE_TENSOR_SIMD=AVX2 但本机不支持 AVX2，已降级为 SCALAR")
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "SSE2" AND NOT BEE_SIMD_HAS_SSE2)
        message(WARNING "BEE_TENSOR_SIMD=SSE2 但本机不支持 SSE2+SSE4.1，已降级为 SCALAR")
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    endif()
endif()

message(STATUS "Bee::Tensor SIMD backend = ${BEE_TENSOR_SIMD_RESOLVED}")

# 若启用 CUDA stub
if(BEE_TENSOR_WITH_CUDA)
    message(STATUS "Bee::Tensor CUDA stub enabled: ON")
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_WITH_CUDA)
endif()

# 设置编译期宏
if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX512")
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_SIMD_AVX512)
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX2")
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_SIMD_AVX2)
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "SSE2")
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_SIMD_SSE2)
else()
    target_compile_definitions(Tensor PUBLIC BEE_TENSOR_SIMD_SCALAR)
endif()

# 设置编译选项（PRIVATE，不传递给消费者）
if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX512")
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX512_FLAGS}>
    )
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX2")
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX2_FLAGS}>
    )
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "SSE2" AND NOT MSVC)
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_SSE2_FLAGS}>
    )
endif()
