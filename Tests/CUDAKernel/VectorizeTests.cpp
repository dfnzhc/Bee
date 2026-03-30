#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "CUDAKernel/Vectorize.cuh"

TEST(CUDAKernelVectorizeTests, PackTraitsWork)
{
    using Pack4f = bee::cuda::Pack<float, 4>;
    EXPECT_EQ((bee::cuda::PackWidthV<Pack4f>), 4);
    EXPECT_EQ(sizeof(Pack4f), sizeof(float) * 4);
}

TEST(CUDAKernelVectorizeTests, CanVectorizePointerAlignment)
{
    using Pack4f = bee::cuda::Pack<float, 4>;

    alignas(16) float aligned[8] = {};
    EXPECT_TRUE((bee::cuda::can_vectorize_ptr<Pack4f>(aligned)));

    alignas(16) std::array<std::byte, 64> raw{};
    const float* misaligned = reinterpret_cast<const float*>(raw.data() + 4);
    EXPECT_FALSE((bee::cuda::can_vectorize_ptr<Pack4f>(misaligned)));
}

TEST(CUDAKernelVectorizeTests, CanVectorizeRequiresEnoughElements)
{
    using Pack4f = bee::cuda::Pack<float, 4>;

    alignas(16) float aligned[8] = {};
    EXPECT_TRUE((bee::cuda::can_vectorize<Pack4f>(aligned, 8)));
    EXPECT_FALSE((bee::cuda::can_vectorize<Pack4f>(aligned, 3)));
}

TEST(CUDAKernelVectorizeTests, ExplicitFloat4PointerCast)
{
    alignas(16) float values[8] = {};
    float* raw_ptr              = values;
    const float* const_raw_ptr  = values;

    float4* vec_ptr             = bee::cuda::as_float4_ptr(raw_ptr);
    const float4* const_vec_ptr = bee::cuda::as_float4_ptr(const_raw_ptr);

    EXPECT_EQ(reinterpret_cast<void*>(vec_ptr), reinterpret_cast<void*>(raw_ptr));
    EXPECT_EQ(reinterpret_cast<const void*>(const_vec_ptr), reinterpret_cast<const void*>(const_raw_ptr));
    EXPECT_TRUE(bee::cuda::is_aligned(vec_ptr, alignof(float4)));
}

TEST(CUDAKernelVectorizeTests, ExplicitInt4PointerCast)
{
    alignas(16) int values[8] = {};
    int* raw_ptr              = values;
    const int* const_raw_ptr  = values;

    int4* vec_ptr             = bee::cuda::as_int4_ptr(raw_ptr);
    const int4* const_vec_ptr = bee::cuda::as_int4_ptr(const_raw_ptr);

    EXPECT_EQ(reinterpret_cast<void*>(vec_ptr), reinterpret_cast<void*>(raw_ptr));
    EXPECT_EQ(reinterpret_cast<const void*>(const_vec_ptr), reinterpret_cast<const void*>(const_raw_ptr));
    EXPECT_TRUE(bee::cuda::is_aligned(vec_ptr, alignof(int4)));
}
