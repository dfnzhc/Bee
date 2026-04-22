bee_add_component_test(
    COMPONENT SIMD
    TARGET SIMD.Tests
    SOURCES
        SimdTests.cpp
)

# SimdTests.cpp 直接引用 SimdBackend<T, IsaAvx2/IsaAvx512> 的成员函数，
# 需要对应编译标志才能展开内部的 AVX2/AVX-512 intrinsic；
# 对测试二进制按本机支持的最高 ISA 加编译标志即可
if(BEE_BUILD_TESTS)
    if(BEE_SIMD_HAS_AVX512)
        target_compile_options(SIMD.Tests PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX512_FLAGS}>
        )
    elseif(BEE_SIMD_HAS_AVX2)
        target_compile_options(SIMD.Tests PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX2_FLAGS}>
        )
    elseif(BEE_SIMD_HAS_SSE2)
        target_compile_options(SIMD.Tests PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_SSE2_FLAGS}>
        )
    endif()
endif()
