set(simd_library_type "STATIC")
set(simd_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE simd_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE simd_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME SIMD
    TYPE "${simd_library_type}"
    SOURCES
        ${simd_sources}
    PUBLIC_HEADERS
        ${simd_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${simd_private_include_dir}"
    DEPENDS
        Bee::Base
)

# ---------------------------------------------------------------------------
# SIMD ISA 能力探测：同时检验编译能力与运行时 CPU 支持
# 使用 check_cxx_source_runs（编译 + 执行）确保 AUTO 模式不在不支持的 CPU 上
# 启用 AVX-512 等高端指令集，避免 illegal instruction (0xc000001d) 崩溃
# 结果以 CACHE 变量导出，供 Bee::Tensor 等组件读取
# ---------------------------------------------------------------------------
include(CheckCXXSourceRuns)

# 根据编译器确定各 ISA 的编译选项
if(MSVC)
    set(BEE_SIMD_AVX512_FLAGS "/arch:AVX512" CACHE STRING "AVX-512 编译标志" FORCE)
    set(BEE_SIMD_AVX2_FLAGS   "/arch:AVX2"   CACHE STRING "AVX2 编译标志" FORCE)
    set(BEE_SIMD_SSE2_FLAGS   ""             CACHE STRING "SSE2 编译标志（MSVC x64 默认开启）" FORCE)
else()
    set(BEE_SIMD_AVX512_FLAGS "-mavx512f -mavx512bw" CACHE STRING "AVX-512F+BW 编译标志" FORCE)
    set(BEE_SIMD_AVX2_FLAGS   "-mavx2"               CACHE STRING "AVX2 编译标志" FORCE)
    set(BEE_SIMD_SSE2_FLAGS   "-msse4.1"             CACHE STRING "SSE2+SSE4.1 编译标志" FORCE)
endif()

set(_bee_simd_saved_flags "${CMAKE_REQUIRED_FLAGS}")

# 探测 AVX-512（需要 F + BW 两个扩展；运行时验证 CPU 实际支持）
set(CMAKE_REQUIRED_FLAGS "${_bee_simd_saved_flags} ${BEE_SIMD_AVX512_FLAGS}")
check_cxx_source_runs(
    "#include <immintrin.h>
int main() {
    __m512  a = _mm512_set1_ps(1.0f);
    __m512i b = _mm512_min_epu8(_mm512_setzero_si512(), _mm512_setzero_si512());
    (void)a; (void)b;
    return 0;
}"
    BEE_SIMD_HAS_AVX512
)

# 探测 AVX2（运行时验证）
set(CMAKE_REQUIRED_FLAGS "${_bee_simd_saved_flags} ${BEE_SIMD_AVX2_FLAGS}")
check_cxx_source_runs(
    "#include <immintrin.h>
int main() { __m256 a = _mm256_set1_ps(1.0f); (void)a; return 0; }"
    BEE_SIMD_HAS_AVX2
)

# 探测 SSE2+SSE4.1（SSE2 后端依赖 SSE4.1 的部分指令；运行时验证）
set(CMAKE_REQUIRED_FLAGS "${_bee_simd_saved_flags} ${BEE_SIMD_SSE2_FLAGS}")
check_cxx_source_runs(
    "#include <smmintrin.h>
int main() {
    __m128i a = _mm_set1_epi32(1);
    __m128i b = _mm_min_epi32(a, a);
    (void)b;
    return 0;
}"
    BEE_SIMD_HAS_SSE2
)

set(CMAKE_REQUIRED_FLAGS "${_bee_simd_saved_flags}")

message(STATUS "Bee::SIMD 能力探测：SSE2+SSE4.1=${BEE_SIMD_HAS_SSE2}  AVX2=${BEE_SIMD_HAS_AVX2}  AVX512+BW=${BEE_SIMD_HAS_AVX512}")
