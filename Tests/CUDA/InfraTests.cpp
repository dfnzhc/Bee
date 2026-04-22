/**
 * @File InfraTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief M1 基础设施层 host 端测试：Stream/Event/MemoryPool RAII + 基本同步。
 */

#include <gtest/gtest.h>

#include "CUDA/Api.hpp"
#include "CUDA/Core/Stream.hpp"
#include "CUDA/Core/Event.hpp"
#include "CUDA/Mem/MemoryPool.hpp"

using namespace bee;

namespace
{

bool has_device()
{
    return cuda::device_count() > 0;
}

} // namespace

TEST(CudaM1Stream, PerThreadViewSynchronize)
{
    if (!has_device()) GTEST_SKIP() << "No CUDA device";
    ASSERT_TRUE(cuda::set_device(0).has_value());

    auto s = cuda::StreamView::per_thread();
    auto r = s.synchronize();
    ASSERT_TRUE(r.has_value()) << r.error().message.data();
}

TEST(CudaM1Stream, OwnedStreamCreateQuerySynchronize)
{
    if (!has_device()) GTEST_SKIP() << "No CUDA device";
    ASSERT_TRUE(cuda::set_device(0).has_value());

    auto s = cuda::OwnedStream::create();
    ASSERT_TRUE(s.has_value()) << s.error().message.data();
    EXPECT_TRUE(static_cast<bool>(s.value()));

    auto q = s->view().query();
    ASSERT_TRUE(q.has_value());
    EXPECT_TRUE(q.value());

    auto sync = s->synchronize();
    ASSERT_TRUE(sync.has_value());
}

TEST(CudaM1Event, RecordAndSynchronize)
{
    if (!has_device()) GTEST_SKIP() << "No CUDA device";
    ASSERT_TRUE(cuda::set_device(0).has_value());

    auto ev = cuda::Event::create();
    ASSERT_TRUE(ev.has_value()) << ev.error().message.data();

    ASSERT_TRUE(ev->record().has_value());
    ASSERT_TRUE(ev->synchronize().has_value());

    auto q = ev->query();
    ASSERT_TRUE(q.has_value());
    EXPECT_TRUE(q.value());
}

TEST(CudaM1MemoryPool, DefaultPoolAllocateFreeAsync)
{
    if (!has_device()) GTEST_SKIP() << "No CUDA device";
    ASSERT_TRUE(cuda::set_device(0).has_value());

    auto pool_r = cuda::MemoryPool::get_default(0);
    ASSERT_TRUE(pool_r.has_value()) << pool_r.error().message.data();
    auto* pool = pool_r.value();
    ASSERT_NE(pool, nullptr);

    auto s = cuda::StreamView::per_thread();
    auto p = pool->allocate_async(1024, s);
    ASSERT_TRUE(p.has_value()) << p.error().message.data();
    ASSERT_NE(p.value(), nullptr);

    pool->deallocate_async(p.value(), s);
    ASSERT_TRUE(s.synchronize().has_value());
}

TEST(CudaM1MemoryPool, ZeroSizeAllocateReturnsNull)
{
    if (!has_device()) GTEST_SKIP() << "No CUDA device";
    ASSERT_TRUE(cuda::set_device(0).has_value());

    auto pool_r = cuda::MemoryPool::get_default(0);
    ASSERT_TRUE(pool_r.has_value());

    auto p = pool_r.value()->allocate_async(0, cuda::StreamView::per_thread());
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p.value(), nullptr);
}
