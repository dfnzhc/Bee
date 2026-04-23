/**
 * @File GemmTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * 专门针对新 AVX2/SSE2/Scalar GEMM 的正确性测试：
 * - 覆盖 F32/F64/I32/I8 四种 dtype
 * - 含非对齐尺寸（触发 M/N 尾部标量路径）
 * - 大矩阵触发三层分块（AVX2 MC=192, KC=384, NC=2048）
 * - I8 → I32 输出 dtype 转换
 */

#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#define ASSERT_OK(expr)  ASSERT_TRUE((expr).has_value())
#define ASSERT_ERR(expr) ASSERT_FALSE((expr).has_value())

using namespace bee;

namespace
{

template <typename TA, typename TC>
auto ref_gemm(int64_t M, int64_t K, int64_t N, const TA* A, const TA* B, TC* C) -> void
{
    for (int64_t i = 0; i < M; ++i)
        for (int64_t j = 0; j < N; ++j) {
            TC acc = TC{0};
            for (int64_t k = 0; k < K; ++k)
                acc += static_cast<TC>(A[i * K + k]) * static_cast<TC>(B[k * N + j]);
            C[i * N + j] = acc;
        }
}

template <typename T>
auto fill_random(std::vector<T>& v, std::mt19937& rng, T lo, T hi) -> void
{
    if constexpr (std::is_floating_point_v<T>) {
        std::uniform_real_distribution<T> dist(lo, hi);
        for (auto& x : v)
            x = dist(rng);
    } else {
        std::uniform_int_distribution<int32_t> dist(static_cast<int32_t>(lo), static_cast<int32_t>(hi));
        for (auto& x : v)
            x = static_cast<T>(dist(rng));
    }
}

template <typename T, DType DT>
auto run_gemm_case(int64_t M, int64_t K, int64_t N, std::mt19937& rng, double tol) -> void
{
    std::vector<T> ra(static_cast<size_t>(M * K));
    std::vector<T> rb(static_cast<size_t>(K * N));
    fill_random(ra, rng, T{-4}, T{4});
    fill_random(rb, rng, T{-4}, T{4});

    std::vector<T> ref(static_cast<size_t>(M * N));
    ref_gemm<T, T>(M, K, N, ra.data(), rb.data(), ref.data());

    auto a = Tensor::empty({M, K}, DT);
    auto b = Tensor::empty({K, N}, DT);
    ASSERT_OK(a);
    ASSERT_OK(b);
    std::memcpy(a->data_ptr(), ra.data(), ra.size() * sizeof(T));
    std::memcpy(b->data_ptr(), rb.data(), rb.size() * sizeof(T));

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    ASSERT_EQ(c->shape(), (Shape{M, N}));
    ASSERT_EQ(c->dtype(), DT);

    const T* pc = static_cast<const T*>(c->data_ptr());
    for (int64_t i = 0; i < M * N; ++i) {
        if constexpr (std::is_floating_point_v<T>) {
            EXPECT_NEAR(pc[i], ref[i], tol) << "mismatch at idx=" << i << " M=" << M << " K=" << K << " N=" << N;
        } else {
            EXPECT_EQ(pc[i], ref[i]) << "mismatch at idx=" << i << " M=" << M << " K=" << K << " N=" << N;
        }
    }
}

} // namespace

// ── F32 GEMM：主干对齐 + 小矩阵 + 非对齐尾部 + 大矩阵分块 ────────────────────
TEST(GemmTests, F32_Sizes)
{
    std::mt19937 rng(0xC0FFEE);
    for (auto [M, K, N] : std::vector<std::tuple<int64_t, int64_t, int64_t>>{
             {8, 8, 8}, {16, 16, 16}, {7, 5, 3}, {9, 11, 13}, {65, 31, 127}, {64, 64, 64}, {200, 200, 200}
         }) {
        run_gemm_case<float, DType::F32>(M, K, N, rng, 1e-3);
    }
}

// ── F64 GEMM ─────────────────────────────────────────────────────────────────
TEST(GemmTests, F64_Sizes)
{
    std::mt19937 rng(0xBADF00D);
    for (auto [M, K, N] :
         std::vector<std::tuple<int64_t, int64_t, int64_t>>{{8, 8, 4}, {16, 16, 16}, {7, 5, 3}, {9, 11, 13}, {65, 31, 127}, {200, 100, 150}}) {
        run_gemm_case<double, DType::F64>(M, K, N, rng, 1e-9);
    }
}

// ── I32 GEMM（精确相等）───────────────────────────────────────────────────────
TEST(GemmTests, I32_Sizes)
{
    std::mt19937 rng(0xDEADBEEF);
    for (auto [M, K, N] :
         std::vector<std::tuple<int64_t, int64_t, int64_t>>{{8, 8, 8}, {4, 4, 4}, {7, 5, 3}, {9, 11, 13}, {65, 31, 127}, {128, 128, 128}}) {
        run_gemm_case<int32_t, DType::I32>(M, K, N, rng, 0.0);
    }
}

// ── 大矩阵触发 AVX2 分块（MC=192, KC=384, NC=2048）──────────────────────────
TEST(GemmTests, F32_LargeBlocked)
{
    std::mt19937 rng(0x12345);
    run_gemm_case<float, DType::F32>(256, 256, 256, rng, 1e-2);
}

// ── I8 GEMM：输入 I8，输出 I32 ────────────────────────────────────────────────
TEST(GemmTests, I8_Basic)
{
    // [[1,2,3],[4,5,6]] × [[1,0],[0,1],[1,1]] = [[1+0+3, 0+2+3],[4+0+6, 0+5+6]]
    //   = [[4,5],[10,11]]
    auto a = Tensor::empty({2, 3}, DType::I8);
    auto b = Tensor::empty({3, 2}, DType::I8);
    ASSERT_OK(a);
    ASSERT_OK(b);
    int8_t* pa = static_cast<int8_t*>(a->data_ptr());
    int8_t* pb = static_cast<int8_t*>(b->data_ptr());
    for (int i = 0; i < 6; ++i)
        pa[i] = static_cast<int8_t>(i + 1);
    pb[0] = 1;
    pb[1] = 0;
    pb[2] = 0;
    pb[3] = 1;
    pb[4] = 1;
    pb[5] = 1;

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->dtype(), DType::I32);
    EXPECT_EQ(c->shape(), (Shape{2, 2}));
    const int32_t* pc = static_cast<const int32_t*>(c->data_ptr());
    EXPECT_EQ(pc[0], 4);
    EXPECT_EQ(pc[1], 5);
    EXPECT_EQ(pc[2], 10);
    EXPECT_EQ(pc[3], 11);
}

TEST(GemmTests, I8_RandomSizes)
{
    std::mt19937 rng(0xABCDEF);
    for (auto [M, K, N] : std::vector<std::tuple<int64_t, int64_t, int64_t>>{{8, 8, 8}, {4, 4, 4}, {7, 5, 3}, {9, 11, 13}, {65, 31, 127}}) {
        std::vector<int8_t> ra(static_cast<size_t>(M * K));
        std::vector<int8_t> rb(static_cast<size_t>(K * N));
        // 限制 |a|,|b| <= 8 以保证 int32 累加不溢出
        fill_random<int8_t>(ra, rng, static_cast<int8_t>(-8), static_cast<int8_t>(8));
        fill_random<int8_t>(rb, rng, static_cast<int8_t>(-8), static_cast<int8_t>(8));

        std::vector<int32_t> ref(static_cast<size_t>(M * N));
        ref_gemm<int8_t, int32_t>(M, K, N, ra.data(), rb.data(), ref.data());

        auto a = Tensor::empty({M, K}, DType::I8);
        auto b = Tensor::empty({K, N}, DType::I8);
        ASSERT_OK(a);
        ASSERT_OK(b);
        std::memcpy(a->data_ptr(), ra.data(), ra.size());
        std::memcpy(b->data_ptr(), rb.data(), rb.size());

        auto c = matmul(*a, *b);
        ASSERT_OK(c);
        EXPECT_EQ(c->dtype(), DType::I32);
        EXPECT_EQ(c->shape(), (Shape{M, N}));
        const int32_t* pc = static_cast<const int32_t*>(c->data_ptr());
        for (int64_t i = 0; i < M * N; ++i)
            EXPECT_EQ(pc[i], ref[i]) << "I8 mismatch idx=" << i << " M=" << M << " K=" << K << " N=" << N;
    }
}
