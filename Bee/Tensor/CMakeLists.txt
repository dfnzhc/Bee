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
)

# ---------------------------------------------------------------------------
# SIMD 后端选项与自动探测
# ---------------------------------------------------------------------------
set(BEE_TENSOR_SIMD "AUTO" CACHE STRING "Tensor SIMD 后端（AUTO/SCALAR/SSE2/AVX2/AVX512）")
set_property(CACHE BEE_TENSOR_SIMD PROPERTY STRINGS AUTO SCALAR SSE2 AVX2 AVX512)

# 校验用户输入的 BEE_TENSOR_SIMD 值
string(TOUPPER "${BEE_TENSOR_SIMD}" _bee_simd_input)
if(NOT _bee_simd_input MATCHES "^(AUTO|SCALAR|SSE2|AVX2|AVX512)$")
    message(FATAL_ERROR "BEE_TENSOR_SIMD 非法值：'${BEE_TENSOR_SIMD}'，期望 AUTO/SCALAR/SSE2/AVX2/AVX512 之一")
endif()

include(CheckCXXSourceCompiles)

# 根据编译器类型确定各 ISA 对应的编译标志
if(MSVC)
    set(_bee_flag_avx512 "/arch:AVX512")
    set(_bee_flag_avx2   "/arch:AVX2")
    set(_bee_flag_sse2   "")   # MSVC x64 默认开启 SSE2
else()
    set(_bee_flag_avx512 "-mavx512f")
    set(_bee_flag_avx2   "-mavx2")
    set(_bee_flag_sse2   "-msse2")
endif()

if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AUTO" OR BEE_TENSOR_SIMD_RESOLVED STREQUAL "")
    # 实际上这里应该由上面的校验保证不会是空的或非法的，但为了安全性保留逻辑
    set(BEE_TENSOR_SIMD_RESOLVED "AUTO")
endif()

if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AUTO")
    # 保存原始 CMAKE_REQUIRED_FLAGS
    set(_bee_saved_flags "${CMAKE_REQUIRED_FLAGS}")

    # 探测 AVX-512
    set(CMAKE_REQUIRED_FLAGS "${_bee_saved_flags} ${_bee_flag_avx512}")
    check_cxx_source_compiles(
        "#include <immintrin.h>
int main() { __m512 a = _mm512_set1_ps(1.0f); (void)a; return 0; }"
        BEE_HAS_AVX512
    )

    # 探测 AVX2
    set(CMAKE_REQUIRED_FLAGS "${_bee_saved_flags} ${_bee_flag_avx2}")
    check_cxx_source_compiles(
        "#include <immintrin.h>
int main() { __m256 a = _mm256_set1_ps(1.0f); (void)a; return 0; }"
        BEE_HAS_AVX2
    )

    # 探测 SSE2
    set(CMAKE_REQUIRED_FLAGS "${_bee_saved_flags} ${_bee_flag_sse2}")
    check_cxx_source_compiles(
        "#include <immintrin.h>
int main() { __m128 a = _mm_set1_ps(1.0f); (void)a; return 0; }"
        BEE_HAS_SSE2
    )

    # 恢复
    set(CMAKE_REQUIRED_FLAGS "${_bee_saved_flags}")

    # 按最高优先级选择：本任务 AVX512/SSE2 无后端，均降级为 AVX2 或 SCALAR
    if(BEE_HAS_AVX2)
        set(BEE_TENSOR_SIMD_RESOLVED "AVX2")
    elseif(BEE_HAS_SSE2)
        message(STATUS "Bee::Tensor: 检测到 SSE2 但尚无 SSE2 后端，降级为 SCALAR")
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    else()
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    endif()
else()
    # 用户手动指定，使用之前校验过的 _bee_simd_input
    set(BEE_TENSOR_SIMD_RESOLVED "${_bee_simd_input}")
    # SSE2/AVX512 手动指定时同样降级（无对应后端）
    if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "SSE2" OR BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX512")
        message(STATUS "Bee::Tensor: ${BEE_TENSOR_SIMD_RESOLVED} 后端尚未实现，降级为 SCALAR")
        set(BEE_TENSOR_SIMD_RESOLVED "SCALAR")
    endif()
endif()

message(STATUS "Bee::Tensor SIMD backend = ${BEE_TENSOR_SIMD_RESOLVED}")

# 若启用 CUDA stub，打印状态信息
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

# 设置编译选项（仅作用于 Tensor target 的 CXX 源文件，使用 PRIVATE 不传递给消费者）
if(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX2")
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${_bee_flag_avx2}>
    )
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "AVX512")
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${_bee_flag_avx512}>
    )
elseif(BEE_TENSOR_SIMD_RESOLVED STREQUAL "SSE2" AND NOT MSVC)
    target_compile_options(Tensor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${_bee_flag_sse2}>
    )
endif()
