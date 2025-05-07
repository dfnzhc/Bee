/**
 * @File PlainMemorySourceTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/30
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee.hpp>

#include <Core/Memory/MemorySource.hpp>

using namespace bee;

class PlainMemorySourceTest : public ::testing::Test
{
protected:
    PlainMemorySource source; // Create an instance of the class for each test
};

// Test basic allocation and deallocation
TEST_F(PlainMemorySourceTest, BasicAllocationDeallocation)
{
    const size_t alloc_size = 128;
    void* ptr               = source.allocate(alloc_size, 16);

    ASSERT_NE(ptr, nullptr); // Check if allocation succeeded

    AllocStats stats_after_alloc = source.stats();
    EXPECT_EQ(stats_after_alloc.allocationCount, 1);
    EXPECT_EQ(stats_after_alloc.deallocationCount, 0);
    EXPECT_EQ(stats_after_alloc.currentUsageBytes, alloc_size);
    EXPECT_EQ(stats_after_alloc.totalAllocatedBytes, alloc_size);
    EXPECT_EQ(stats_after_alloc.peakUsageBytes, alloc_size);

    source.deallocate(ptr, alloc_size);

    AllocStats stats_after_dealloc = source.stats();
    EXPECT_EQ(stats_after_dealloc.allocationCount, 1);
    EXPECT_EQ(stats_after_dealloc.deallocationCount, 1);
    EXPECT_EQ(stats_after_dealloc.currentUsageBytes, 0);
    EXPECT_EQ(stats_after_dealloc.totalAllocatedBytes, alloc_size); // Total remains
    EXPECT_EQ(stats_after_dealloc.peakUsageBytes, alloc_size);      // Peak remains
}

// Test aligned allocation
TEST_F(PlainMemorySourceTest, AlignedAllocation)
{
    const size_t alloc_size = 256;
    const size_t alignment  = 64; // Choose an alignment larger than default potentially

    ASSERT_TRUE(IsPowerOfTwo(alignment)) << "Alignment must be a power of two";

    void* ptr = source.allocate(alloc_size, alignment);
    ASSERT_NE(ptr, nullptr);

    // Check alignment
    uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(ptr_val % alignment, 0) << "Pointer " << ptr << " is not aligned to " << alignment;

    source.deallocate(ptr, alloc_size);

    EXPECT_EQ(source.stats().currentUsageBytes, 0);
    EXPECT_EQ(source.stats().allocationCount, 1);
    EXPECT_EQ(source.stats().deallocationCount, 1);
}

// Test allocating zero bytes
TEST_F(PlainMemorySourceTest, ZeroSizeAllocation)
{
    // Requesting zero bytes should ideally return nullptr
    void* ptr = source.allocate(0, 16);
    EXPECT_EQ(ptr, nullptr);

    // Stats should not change for a failed/zero allocation
    AllocStats stats = source.stats();
    EXPECT_EQ(stats.allocationCount, 0);
    EXPECT_EQ(stats.deallocationCount, 0);
    EXPECT_EQ(stats.currentUsageBytes, 0);
    EXPECT_EQ(stats.totalAllocatedBytes, 0);
    EXPECT_EQ(stats.peakUsageBytes, 0);
}

// Test deallocating a null pointer
TEST_F(PlainMemorySourceTest, DeallocateNull)
{
    // Deallocating nullptr should be safe and a no-op
    EXPECT_NO_THROW(source.deallocate(nullptr, 0));

    // Stats should remain unchanged
    AllocStats stats = source.stats();
    EXPECT_EQ(stats.allocationCount, 0);
    EXPECT_EQ(stats.deallocationCount, 0); // Deallocating null doesn't count
    EXPECT_EQ(stats.currentUsageBytes, 0);
}

// Test multiple allocations and deallocations for stats accuracy
TEST_F(PlainMemorySourceTest, MultipleAllocationsStats)
{
    const size_t size1 = 64;
    const size_t size2 = 128;
    const size_t size3 = 256;

    void* ptr1 = source.allocate(size1, 8);
    void* ptr2 = source.allocate(size2, 16);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    AllocStats stats1 = source.stats();
    EXPECT_EQ(stats1.allocationCount, 2);
    EXPECT_EQ(stats1.deallocationCount, 0);
    EXPECT_EQ(stats1.currentUsageBytes, size1 + size2);
    EXPECT_EQ(stats1.totalAllocatedBytes, size1 + size2);
    EXPECT_EQ(stats1.peakUsageBytes, size1 + size2);

    source.deallocate(ptr1, size1);

    AllocStats stats2 = source.stats();
    EXPECT_EQ(stats2.allocationCount, 2);
    EXPECT_EQ(stats2.deallocationCount, 1);
    EXPECT_EQ(stats2.currentUsageBytes, size2);           // Only ptr2 remains
    EXPECT_EQ(stats2.totalAllocatedBytes, size1 + size2); // Total remains
    EXPECT_EQ(stats2.peakUsageBytes, size1 + size2);      // Peak remains

    void* ptr3 = source.allocate(size3, 32);
    ASSERT_NE(ptr3, nullptr);

    AllocStats stats3 = source.stats();
    EXPECT_EQ(stats3.allocationCount, 3);
    EXPECT_EQ(stats3.deallocationCount, 1);
    EXPECT_EQ(stats3.currentUsageBytes, size2 + size3);
    EXPECT_EQ(stats3.totalAllocatedBytes, size1 + size2 + size3);
    // Peak usage could be size1+size2 or size2+size3, depending on which is larger
    EXPECT_EQ(stats3.peakUsageBytes, std::max(size1 + size2, size2 + size3));

    source.deallocate(ptr2, size2);
    source.deallocate(ptr3, size3);

    AllocStats stats_final = source.stats();
    EXPECT_EQ(stats_final.allocationCount, 3);
    EXPECT_EQ(stats_final.deallocationCount, 3);
    EXPECT_EQ(stats_final.currentUsageBytes, 0);
    EXPECT_EQ(stats_final.totalAllocatedBytes, size1 + size2 + size3);
    EXPECT_EQ(stats_final.peakUsageBytes, std::max(size1 + size2, size2 + size3));
}

// Test basic thread safety of statistics updates
TEST_F(PlainMemorySourceTest, BasicThreadSafetyStats)
{
    const int num_threads            = 4;
    const int allocations_per_thread = 1000;
    const size_t alloc_size          = 32;

    std::vector<std::thread> threads;
    std::vector<void*> pointers[num_threads]; // Store pointers per thread

    for (int i = 0; i < num_threads; ++i) {
        pointers[i].reserve(allocations_per_thread);
        threads.emplace_back([this, i, &pointers] {
            for (int j = 0; j < allocations_per_thread; ++j) {
                void* ptr = source.allocate(alloc_size, 8);
                // In a real scenario, check for nullptr, but for testing assume it works
                if (ptr) {
                    pointers[i].push_back(ptr);
                }
                // Simulate some work / delay if needed
                // std::this_thread::yield();
            }
            // Deallocate only the pointers allocated by this thread
            for (void* ptr : pointers[i]) {
                source.deallocate(ptr, alloc_size);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify final statistics
    AllocStats final_stats = source.stats();
    size_t total_allocations    = num_threads * allocations_per_thread;

    // Allow for potential allocation failures if ptr == nullptr check was active
    size_t successful_allocations = 0;
    for (int i = 0; i < num_threads; ++i)
        successful_allocations += pointers[i].size();

    EXPECT_EQ(final_stats.currentUsageBytes, 0);
    // Check if the total counts match the expected number of operations
    // This relies on allocations succeeding in the threads.
    // If allocate could fail, checking exact counts is harder without tracking failures.
    // Assuming all allocations succeeded for simplicity here:
    EXPECT_EQ(final_stats.allocationCount, total_allocations);
    EXPECT_EQ(final_stats.deallocationCount, total_allocations);
    EXPECT_EQ(final_stats.totalAllocatedBytes, total_allocations * alloc_size);
    // Peak usage should be at most total_allocations * alloc_size, but could be less
    // depending on thread interleaving. Checking exact peak is hard.
    // We can at least check it's >= max concurrent usage by one thread
    // and <= total possible usage.
    EXPECT_GE(final_stats.peakUsageBytes, allocations_per_thread * alloc_size);
    EXPECT_LE(final_stats.peakUsageBytes, total_allocations * alloc_size);
}