#include "gtest/gtest.h"

#include "Tensor/Tensor.hpp"
#include "Tensor/Cuda/CudaAllocator.hpp"

#include <vector>

using namespace bee;

#if !defined(BEE_TENSOR_WITH_CUDA)

// ── BEE_TENSOR_WITH_CUDA=OFF：验证 stub 路径 ─────────────────────────────────

TEST(CudaStubTests, AllocateReturnsNotAvailable)
{
    auto result = tensor::cuda::allocate(16, 64);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：allocate");
}

TEST(CudaStubTests, MemcpyH2dReturnsNotAvailable)
{
    char dummy_src[16] = {};
    char dummy_dst[16] = {};
    auto result        = tensor::cuda::memcpy_h2d(dummy_dst, dummy_src, 16);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：memcpy_h2d");
}

TEST(CudaStubTests, CudaAllocatorDeviceIsCuda)
{
    auto& alloc = CudaAllocator::instance();
    EXPECT_EQ(alloc.device(), Device::CUDA);
}

TEST(CudaStubTests, TensorEmptyWithCudaFails)
{
    auto result = Tensor::empty({4}, DType::F32, Device::CUDA);
    ASSERT_FALSE(result);
}

#else // BEE_TENSOR_WITH_CUDA

// ── BEE_TENSOR_WITH_CUDA=ON：验证真实 CUDA 后端端到端 ──────────────────────────

    #include "CUDA/Api.hpp"

namespace
{

bool cuda_available()
{
    return cuda::device_count() > 0;
}

} // namespace

TEST(CudaBackend, CudaAllocatorDeviceIsCuda)
{
    auto& alloc = CudaAllocator::instance();
    EXPECT_EQ(alloc.device(), Device::CUDA);
}

TEST(CudaBackend, EmptyOnCudaAllocatesDevicePointer)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::empty({16, 8}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();
    EXPECT_EQ(t->device(), Device::CUDA);
    EXPECT_EQ(t->numel(), 16 * 8);
    EXPECT_TRUE(t->is_contiguous());
    EXPECT_NE(t->data_ptr(), nullptr);
}

TEST(CudaBackend, ZerosOnCudaIsAllZero)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::size_t N = 64;
    auto                  t = Tensor::zeros({static_cast<std::int64_t>(N)}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();

    auto cpu_r = t->to(Device::CPU);
    ASSERT_TRUE(cpu_r.has_value()) << cpu_r.error().message.view();
    const auto* ptr = static_cast<const float*>(cpu_r->data_ptr());
    for (std::size_t i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 0.0f);
}

TEST(CudaBackend, ToDeviceRoundTripPreservesValues)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";

    constexpr std::int64_t N   = 128;
    auto                   cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU);
    ASSERT_TRUE(cpu.has_value());

    auto on_cuda = cpu->to(Device::CUDA);
    ASSERT_TRUE(on_cuda.has_value()) << on_cuda.error().message.view();
    EXPECT_EQ(on_cuda->device(), Device::CUDA);

    auto back = on_cuda->to(Device::CPU);
    ASSERT_TRUE(back.has_value()) << back.error().message.view();
    EXPECT_EQ(back->device(), Device::CPU);

    const auto* src = static_cast<const float*>(cpu->data_ptr());
    const auto* dst = static_cast<const float*>(back->data_ptr());
    for (std::int64_t i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(dst[i], src[i]) << "i=" << i;
}

TEST(CudaBackend, ArangeOnCudaWorksViaCpuBridge)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::arange(0, 32, 1, DType::I32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();
    EXPECT_EQ(t->device(), Device::CUDA);

    auto back = t->to(Device::CPU);
    ASSERT_TRUE(back.has_value());
    const auto* ptr = static_cast<const std::int32_t*>(back->data_ptr());
    for (std::int32_t i = 0; i < 32; ++i)
        EXPECT_EQ(ptr[i], i);
}

TEST(CudaBackend, CloneOnCudaProducesIndependentStorage)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto a = Tensor::arange(0, 16, 1, DType::F32, Device::CUDA);
    ASSERT_TRUE(a.has_value());
    auto b = a->clone();
    ASSERT_TRUE(b.has_value()) << b.error().message.view();
    EXPECT_EQ(b->device(), Device::CUDA);
    EXPECT_NE(b->data_ptr(), a->data_ptr());

    auto a_cpu = a->to(Device::CPU);
    auto b_cpu = b->to(Device::CPU);
    ASSERT_TRUE(a_cpu.has_value());
    ASSERT_TRUE(b_cpu.has_value());
    const auto* pa = static_cast<const float*>(a_cpu->data_ptr());
    const auto* pb = static_cast<const float*>(b_cpu->data_ptr());
    for (int i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ(pa[i], pb[i]);
}

TEST(CudaBackend, ToSameDeviceReturnsShallowCopy)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::zeros({4}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value());
    auto same = t->to(Device::CUDA);
    ASSERT_TRUE(same.has_value());
    EXPECT_EQ(same->data_ptr(), t->data_ptr());
}

// ── M3：ElementWise / Cast CUDA 数值对拍 ─────────────────────────────────────

namespace
{

auto to_cpu_floats(const Tensor& t) -> std::vector<float>
{
    auto        cpu = t.to(Device::CPU).value();
    const auto* p   = static_cast<const float*>(cpu.data_ptr());
    return std::vector<float>(p, p + cpu.numel());
}

} // namespace

TEST(CudaOps, BinaryAddF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N     = 257;
    auto                   a_cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU).value();
    auto                   b_cpu = Tensor::arange(N, 2 * N, 1, DType::F32, Device::CPU).value();
    auto                   c_cpu = add(a_cpu, b_cpu).value();

    auto a_cu = a_cpu.to(Device::CUDA).value();
    auto b_cu = b_cpu.to(Device::CUDA).value();
    auto c_cu = add(a_cu, b_cu).value();
    EXPECT_EQ(c_cu.device(), Device::CUDA);

    auto        lhs = to_cpu_floats(c_cu);
    const auto* r   = static_cast<const float*>(c_cpu.data_ptr());
    for (std::int64_t i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(lhs[i], r[i]);
}

TEST(CudaOps, BinarySubMulDivF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N     = 131;
    auto                   a_cpu = Tensor::arange(1, N + 1, 1, DType::F32, Device::CPU).value();
    auto                   b_cpu = Tensor::full({N}, DType::F32, 2.0, Device::CPU).value();
    auto                   a_cu  = a_cpu.to(Device::CUDA).value();
    auto                   b_cu  = b_cpu.to(Device::CUDA).value();

    auto sub_cpu = sub(a_cpu, b_cpu).value();
    auto sub_cu  = sub(a_cu, b_cu).value();
    auto mul_cpu = mul(a_cpu, b_cpu).value();
    auto mul_cu  = mul(a_cu, b_cu).value();
    auto div_cpu = div(a_cpu, b_cpu).value();
    auto div_cu  = div(a_cu, b_cu).value();

    auto check = [&](const Tensor& cpu, const Tensor& cu) {
        auto        gpu = to_cpu_floats(cu);
        const auto* r   = static_cast<const float*>(cpu.data_ptr());
        for (std::int64_t i = 0; i < N; ++i)
            EXPECT_FLOAT_EQ(gpu[i], r[i]) << "i=" << i;
    };
    check(sub_cpu, sub_cu);
    check(mul_cpu, mul_cu);
    check(div_cpu, div_cu);
}

TEST(CudaOps, UnaryNegAbsSqrtExpLogF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N       = 64;
    auto                   src_cpu = Tensor::arange(1, N + 1, 1, DType::F32, Device::CPU).value();
    auto                   src_cu  = src_cpu.to(Device::CUDA).value();

    auto cmp = [&](const Tensor& cpu, const Tensor& cu, float tol) {
        auto        gpu = to_cpu_floats(cu);
        const auto* r   = static_cast<const float*>(cpu.data_ptr());
        for (std::int64_t i = 0; i < N; ++i)
            EXPECT_NEAR(gpu[i], r[i], tol) << "i=" << i;
    };

    cmp(neg(src_cpu).value(), neg(src_cu).value(), 0.0f);
    cmp(abs(src_cpu).value(), abs(src_cu).value(), 0.0f);
    cmp(sqrt(src_cpu).value(), sqrt(src_cu).value(), 1e-5f);
    // exp over 1..64 溢出；只对前 8 个元素对拍。
    {
        auto        small_cpu = Tensor::arange(0, 8, 1, DType::F32, Device::CPU).value();
        auto        small_cu  = small_cpu.to(Device::CUDA).value();
        auto        e_cpu     = exp(small_cpu).value();
        auto        e_cu      = exp(small_cu).value();
        auto        gpu       = to_cpu_floats(e_cu);
        const auto* r         = static_cast<const float*>(e_cpu.data_ptr());
        for (int i = 0; i < 8; ++i)
            EXPECT_NEAR(gpu[i], r[i], std::abs(r[i]) * 1e-4f);
    }
    cmp(log(src_cpu).value(), log(src_cu).value(), 1e-5f);
}

TEST(CudaOps, InplaceAddF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N     = 100;
    auto                   a_cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU).value();
    auto                   b_cpu = Tensor::full({N}, DType::F32, 3.0, Device::CPU).value();
    auto                   a_cu  = a_cpu.to(Device::CUDA).value();
    auto                   b_cu  = b_cpu.to(Device::CUDA).value();

    ASSERT_TRUE(add_inplace(a_cpu, b_cpu).has_value());
    ASSERT_TRUE(add_inplace(a_cu, b_cu).has_value());

    auto        gpu = to_cpu_floats(a_cu);
    const auto* r   = static_cast<const float*>(a_cpu.data_ptr());
    for (std::int64_t i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(gpu[i], r[i]);
}

TEST(CudaOps, CastF32ToI32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N       = 32;
    auto                   src_cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU).value();
    auto                   src_cu  = src_cpu.to(Device::CUDA).value();

    auto dst_cpu = cast(src_cpu, DType::I32).value();
    auto dst_cu  = cast(src_cu, DType::I32).value();

    auto        back = dst_cu.to(Device::CPU).value();
    const auto* g    = static_cast<const std::int32_t*>(back.data_ptr());
    const auto* r    = static_cast<const std::int32_t*>(dst_cpu.data_ptr());
    for (std::int64_t i = 0; i < N; ++i)
        EXPECT_EQ(g[i], r[i]);
}

TEST(CudaOps, CastI32ToF64MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N       = 48;
    auto                   src_cpu = Tensor::arange(-16, N - 16, 1, DType::I32, Device::CPU).value();
    auto                   src_cu  = src_cpu.to(Device::CUDA).value();

    auto dst_cpu = cast(src_cpu, DType::F64).value();
    auto dst_cu  = cast(src_cu, DType::F64).value();

    auto        back = dst_cu.to(Device::CPU).value();
    const auto* g    = static_cast<const double*>(back.data_ptr());
    const auto* r    = static_cast<const double*>(dst_cpu.data_ptr());
    for (std::int64_t i = 0; i < N; ++i)
        EXPECT_DOUBLE_EQ(g[i], r[i]);
}

TEST(CudaOps, BinaryShapeMismatchReturnsError)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto a = Tensor::zeros({4, 3}, DType::F32, Device::CUDA).value();
    auto b = Tensor::zeros({3}, DType::F32, Device::CUDA).value();
    auto r = add(a, b);
    EXPECT_FALSE(r.has_value());
}

// ── M4：Reduce / Random CUDA 对拍 ───────────────────────────────────────────

TEST(CudaOps, ReduceGlobalSumF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N   = 257;
    auto                   cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU).value();
    auto                   cu  = cpu.to(Device::CUDA).value();

    auto s_cpu = sum(cpu).value();
    auto s_cu  = sum(cu).value();
    auto back  = s_cu.to(Device::CPU).value();
    EXPECT_FLOAT_EQ(*static_cast<const float*>(back.data_ptr()), *static_cast<const float*>(s_cpu.data_ptr()));
}

TEST(CudaOps, ReduceGlobalMinMaxProdI32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto cpu = Tensor::arange(1, 13, 1, DType::I32, Device::CPU).value(); // 1..12
    auto cu  = cpu.to(Device::CUDA).value();

    auto mn_cpu = min(cpu).value();
    auto mn_cu  = min(cu).value().to(Device::CPU).value();
    auto mx_cpu = max(cpu).value();
    auto mx_cu  = max(cu).value().to(Device::CPU).value();
    auto pr_cpu = prod(cpu).value();
    auto pr_cu  = prod(cu).value().to(Device::CPU).value();

    EXPECT_EQ(*static_cast<const std::int32_t*>(mn_cu.data_ptr()), *static_cast<const std::int32_t*>(mn_cpu.data_ptr()));
    EXPECT_EQ(*static_cast<const std::int32_t*>(mx_cu.data_ptr()), *static_cast<const std::int32_t*>(mx_cpu.data_ptr()));
    EXPECT_EQ(*static_cast<const std::int32_t*>(pr_cu.data_ptr()), *static_cast<const std::int32_t*>(pr_cpu.data_ptr()));
}

TEST(CudaOps, ReduceGlobalMeanF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    constexpr std::int64_t N   = 128;
    auto                   cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU).value();
    auto                   cu  = cpu.to(Device::CUDA).value();

    auto m_cpu = mean(cpu).value();
    auto m_cu  = mean(cu).value().to(Device::CPU).value();
    EXPECT_FLOAT_EQ(*static_cast<const float*>(m_cu.data_ptr()), *static_cast<const float*>(m_cpu.data_ptr()));
}

TEST(CudaOps, ReduceAxisSumF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    // shape [3, 4, 5]，沿 dim=1 累加
    auto cpu = Tensor::arange(0, 60, 1, DType::F32, Device::CPU).value();
    cpu      = cpu.reshape({3, 4, 5}).value();
    auto cu  = cpu.to(Device::CUDA).value();

    auto s_cpu = sum(cpu, 1, /*keepdim=*/false).value();
    auto s_cu  = sum(cu, 1, false).value().to(Device::CPU).value();
    ASSERT_EQ(s_cpu.numel(), s_cu.numel());
    const auto* g = static_cast<const float*>(s_cu.data_ptr());
    const auto* r = static_cast<const float*>(s_cpu.data_ptr());
    for (std::int64_t i = 0; i < s_cpu.numel(); ++i)
        EXPECT_FLOAT_EQ(g[i], r[i]) << "i=" << i;
}

TEST(CudaOps, ReduceAxisMeanF64KeepdimMatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto cpu = Tensor::arange(0, 24, 1, DType::F64, Device::CPU).value();
    cpu      = cpu.reshape({2, 3, 4}).value();
    auto cu  = cpu.to(Device::CUDA).value();

    auto m_cpu = mean(cpu, -1, /*keepdim=*/true).value();
    auto m_cu  = mean(cu, -1, true).value().to(Device::CPU).value();
    ASSERT_EQ(m_cpu.numel(), m_cu.numel());
    const auto* g = static_cast<const double*>(m_cu.data_ptr());
    const auto* r = static_cast<const double*>(m_cpu.data_ptr());
    for (std::int64_t i = 0; i < m_cpu.numel(); ++i)
        EXPECT_DOUBLE_EQ(g[i], r[i]);
}

TEST(CudaOps, ReduceMeanIntOnCudaReturnsNotImplemented)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto cu = Tensor::arange(0, 10, 1, DType::I32, Device::CUDA).value();
    auto r  = mean(cu);
    EXPECT_FALSE(r.has_value());
}

TEST(CudaOps, RandFloatOnCudaShapeAndDevice)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto t = rand({8, 16}, DType::F32, 42, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();
    EXPECT_EQ(t->device(), Device::CUDA);
    EXPECT_EQ(t->numel(), 128);

    auto        cpu = t->to(Device::CPU).value();
    const auto* p   = static_cast<const float*>(cpu.data_ptr());
    for (int i = 0; i < cpu.numel(); ++i) {
        EXPECT_GE(p[i], 0.0f);
        EXPECT_LT(p[i], 1.0f);
    }
}

TEST(CudaOps, RandIntOnCudaSameSeedMatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto        cpu  = randint(-5, 5, {64}, DType::I32, 123, Device::CPU).value();
    auto        cu   = randint(-5, 5, {64}, DType::I32, 123, Device::CUDA).value();
    auto        back = cu.to(Device::CPU).value();
    const auto* g    = static_cast<const std::int32_t*>(back.data_ptr());
    const auto* r    = static_cast<const std::int32_t*>(cpu.data_ptr());
    for (int i = 0; i < 64; ++i)
        EXPECT_EQ(g[i], r[i]);
}

// ── M5 Matmul ───────────────────────────────────────────────────────────────

TEST(CudaOps, MatmulF32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    const int M = 17, K = 33, N = 21;
    auto      a       = randn({M, K}, DType::F32, 1, Device::CPU).value();
    auto      b       = randn({K, N}, DType::F32, 2, Device::CPU).value();
    auto      cpu_res = matmul(a, b).value();

    auto ac     = a.to(Device::CUDA).value();
    auto bc     = b.to(Device::CUDA).value();
    auto cu_res = matmul(ac, bc).value();
    auto back   = cu_res.to(Device::CPU).value();

    const auto* p = static_cast<const float*>(back.data_ptr());
    const auto* r = static_cast<const float*>(cpu_res.data_ptr());
    for (int i = 0; i < M * N; ++i)
        EXPECT_NEAR(p[i], r[i], 1e-3f);
}

TEST(CudaOps, MatmulF64MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    const int M = 8, K = 16, N = 12;
    auto      a       = randn({M, K}, DType::F64, 5, Device::CPU).value();
    auto      b       = randn({K, N}, DType::F64, 6, Device::CPU).value();
    auto      cpu_res = matmul(a, b).value();

    auto cu_res = matmul(a.to(Device::CUDA).value(), b.to(Device::CUDA).value()).value();
    auto back   = cu_res.to(Device::CPU).value();

    const auto* p = static_cast<const double*>(back.data_ptr());
    const auto* r = static_cast<const double*>(cpu_res.data_ptr());
    for (int i = 0; i < M * N; ++i)
        EXPECT_NEAR(p[i], r[i], 1e-9);
}

TEST(CudaOps, MatmulI32MatchesCpu)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    const int M = 5, K = 7, N = 6;
    auto      a       = randint(-3, 3, {M, K}, DType::I32, 11, Device::CPU).value();
    auto      b       = randint(-3, 3, {K, N}, DType::I32, 12, Device::CPU).value();
    auto      cpu_res = matmul(a, b).value();
    auto      cu_res  = matmul(a.to(Device::CUDA).value(), b.to(Device::CUDA).value()).value();
    auto      back    = cu_res.to(Device::CPU).value();

    const auto* p = static_cast<const std::int32_t*>(back.data_ptr());
    const auto* r = static_cast<const std::int32_t*>(cpu_res.data_ptr());
    for (int i = 0; i < M * N; ++i)
        EXPECT_EQ(p[i], r[i]);
}

TEST(CudaOps, MatmulSmallOnTileBoundary)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    // Exercise shapes that are exact multiples of TILE and larger than 1 tile.
    const int   M = 32, K = 48, N = 32;
    auto        a       = randn({M, K}, DType::F32, 7, Device::CPU).value();
    auto        b       = randn({K, N}, DType::F32, 8, Device::CPU).value();
    auto        cpu_res = matmul(a, b).value();
    auto        cu_res  = matmul(a.to(Device::CUDA).value(), b.to(Device::CUDA).value()).value();
    auto        back    = cu_res.to(Device::CPU).value();
    const auto* p       = static_cast<const float*>(back.data_ptr());
    const auto* r       = static_cast<const float*>(cpu_res.data_ptr());
    for (int i = 0; i < M * N; ++i)
        EXPECT_NEAR(p[i], r[i], 1e-3f);
}

// ── M8 Transpose（通过 contiguous() 路径触发） ───────────────────────────────

TEST(CudaOps, Transpose2DViaContiguousF32)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto host   = randn({7, 13}, DType::F32, 42, Device::CPU).value();
    auto host_t = host.transpose(0, 1).value();
    auto host_c = host_t.contiguous().value();

    auto cu   = host.to(Device::CUDA).value();
    auto cu_t = cu.transpose(0, 1).value();
    ASSERT_FALSE(cu_t.is_contiguous());
    auto cu_c = cu_t.contiguous().value();
    ASSERT_TRUE(cu_c.is_contiguous());
    auto back = cu_c.to(Device::CPU).value();

    const auto* p = static_cast<const float*>(back.data_ptr());
    const auto* r = static_cast<const float*>(host_c.data_ptr());
    for (int i = 0; i < host_c.numel(); ++i)
        EXPECT_FLOAT_EQ(p[i], r[i]);
}

TEST(CudaOps, Transpose2DViaContiguousI64)
{
    if (!cuda_available())
        GTEST_SKIP() << "No CUDA device";
    auto host   = randint(-100, 100, {11, 5}, DType::I64, 77, Device::CPU).value();
    auto host_c = host.transpose(0, 1).value().contiguous().value();

    auto cu   = host.to(Device::CUDA).value();
    auto cu_c = cu.transpose(0, 1).value().contiguous().value();
    auto back = cu_c.to(Device::CPU).value();

    const auto* p = static_cast<const std::int64_t*>(back.data_ptr());
    const auto* r = static_cast<const std::int64_t*>(host_c.data_ptr());
    for (int i = 0; i < host_c.numel(); ++i)
        EXPECT_EQ(p[i], r[i]);
}

#endif // BEE_TENSOR_WITH_CUDA
