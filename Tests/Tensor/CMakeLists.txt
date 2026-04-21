bee_add_component_test(
    COMPONENT Tensor
    TARGET Tensor.Tests
    SOURCES
        TensorSkeletonTests.cpp
        DTypeTests.cpp
        ShapeTests.cpp
        StorageTests.cpp
        TensorTests.cpp
        ViewTests.cpp
        SimdTests.cpp
        CreationTests.cpp
        BroadcastTests.cpp
        ElementWiseTests.cpp
        ReduceTests.cpp
        CastTests.cpp
        RandomTests.cpp
        MatmulTests.cpp
        CudaStubTests.cpp
        IntegrationTests.cpp
)

# SimdTests.cpp 直接引用 SimdBackend<T, IsaAvx2/IsaAvx512> 的成员函数，
# 需要对应编译标志才能展开内部的 AVX2/AVX-512 intrinsic；
# 对测试二进制全局启用本机支持的最高 ISA 标志即可
if(BEE_SIMD_HAS_AVX512)
    target_compile_options(Tensor.Tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX512_FLAGS}>
    )
elseif(BEE_SIMD_HAS_AVX2)
    target_compile_options(Tensor.Tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_AVX2_FLAGS}>
    )
elseif(BEE_SIMD_HAS_SSE2)
    target_compile_options(Tensor.Tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${BEE_SIMD_SSE2_FLAGS}>
    )
endif()
