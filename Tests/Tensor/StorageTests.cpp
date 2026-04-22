#include <gtest/gtest.h>

#include "Base/Memory/Allocator.hpp"
#include "Tensor/Core/Storage.hpp"

using namespace bee;

// ── 基本分配测试 ─────────────────────────────────────────────────────────────

TEST(StorageTests, AllocateSuccess)
{
    auto result = Storage::allocate(256, CpuAllocator::instance());
    ASSERT_TRUE(result.has_value());

    const auto& storage = *result;
    EXPECT_NE(storage->data(), nullptr);
    EXPECT_EQ(storage->nbytes(), 256u);
    EXPECT_EQ(storage->device(), Device::CPU);
}

TEST(StorageTests, AllocateZeroBytes)
{
    // 0 字节分配：行为由 operator new 决定，指针可能非 null，nbytes 应为 0
    auto result = Storage::allocate(0, CpuAllocator::instance());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->nbytes(), 0u);
}

TEST(StorageTests, AllocatorReferenceIsPreserved)
{
    auto result = Storage::allocate(64, CpuAllocator::instance());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&(*result)->allocator(), &CpuAllocator::instance());
}

// ── shared_ptr 引用计数测试 ───────────────────────────────────────────────────

TEST(StorageTests, SharedOwnership)
{
    auto result = Storage::allocate(128, CpuAllocator::instance());
    ASSERT_TRUE(result.has_value());

    // 移出 result，避免 result 自身占用一个引用计数
    auto sp1 = std::move(*result);
    {
        auto sp2 = sp1; // 第二个共享者
        EXPECT_EQ(sp1.use_count(), 2);
        EXPECT_EQ(sp1.get(), sp2.get()); // 同一 Storage 对象
    }
    // sp2 离开作用域，仍有 sp1 持有，不应释放
    EXPECT_EQ(sp1.use_count(), 1);
    EXPECT_NE(sp1->data(), nullptr); // 内存仍有效
}

// ── 大分配失败测试 ───────────────────────────────────────────────────────────

TEST(StorageTests, HugeAllocationFails)
{
    // 使用极大值触发 bad_alloc，验证返回 Err 而非抛出异常
    const std::size_t huge   = std::numeric_limits<std::size_t>::max() / 2;
    auto              result = Storage::allocate(huge, CpuAllocator::instance());
    EXPECT_FALSE(result.has_value());
}

// ── 对齐参数匹配测试 ──────────────────────────────────────────────────────────

TEST(StorageTests, NonDefaultAlignmentAllocateDeallocate)
{
    // CpuAllocator 内部强制至少 64 字节对齐，但接口层应接受任意 alignment
    // 此处验证传入 128 时 allocate/deallocate 配对不崩溃
    auto& alloc  = CpuAllocator::instance();
    auto  result = alloc.allocate(128, 128);
    ASSERT_TRUE(result.has_value());
    void* p = result.value();
    EXPECT_NE(p, nullptr);
    alloc.deallocate(p, 128, 128); // 不应崩溃
}
