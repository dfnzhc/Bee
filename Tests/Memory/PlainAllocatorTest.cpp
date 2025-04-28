/**
 * @File MathTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee.hpp>

#include <Memory/Allocators/PlainAllocator.hpp>

using namespace bee;

// --- Allocation and Deallocation Tests ---

TEST(PlainAllocatorTest, AllocateDeallocate_SmallSize)
{
    PlainAllocator allocator;
    Size size = 16; // Small size allocation

    void* ptr = allocator.allocate(size);

    // Allocation should succeed and return a non-null pointer
    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr));
    EXPECT_TRUE(allocator.usableSize(ptr) == size);

    allocator.deallocate(ptr);
}

TEST(PlainAllocatorTest, AllocateDeallocate_MediumSize)
{
    PlainAllocator allocator;
    Size size = 1024; // Medium size allocation

    void* ptr = allocator.allocate(size);

    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr));
    EXPECT_TRUE(allocator.usableSize(ptr) == size);

    allocator.deallocate(ptr);
}

TEST(PlainAllocatorTest, AllocateDeallocate_LargeSize)
{
    PlainAllocator allocator;
    Size size = 1024 * 1024; // 1MB, relatively large size allocation

    void* ptr = allocator.allocate(size);

    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr));
    EXPECT_TRUE(allocator.usableSize(ptr) == size);

    allocator.deallocate(ptr);
}

TEST(PlainAllocatorTest, AllocateDeallocate_ZeroSize)
{
    PlainAllocator allocator;
    Size size = 0;

    void* ptr = allocator.allocate(size);

    // According to mimalloc documentation, mi_malloc(0) typically returns a non-NULL pointer
    // that can be safely deallocated.
    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr));

    // minimum size is 8.
    EXPECT_EQ(allocator.usableSize(ptr), 8);

    allocator.deallocate(ptr);
}

TEST(PlainAllocatorTest, AllocateWithAlignment)
{
    PlainAllocator allocator;
    Size size = 64;
    // Test various power-of-two alignments
    Size alignments[] = {8, 16, 32, 64, 128, 256, 512, 1024};

    for (Size alignment : alignments) {
        // Arbitrary reasonable limit for common systems
        void* ptr = allocator.allocate(size, alignment);

        EXPECT_NE(nullptr, ptr);
        EXPECT_TRUE(allocator.owns(ptr));

        // Check if the returned pointer address is a multiple of the requested alignment
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment, 0);

        allocator.deallocate(ptr);
    }

    // Test with 0 alignment (should typically default to a system-specific or mimalloc default alignment)
    // A common default is 8 or 16 bytes. We can check for a minimum common alignment.
    void* ptr_default = allocator.allocate(size, 0);
    EXPECT_NE(nullptr, ptr_default);
    EXPECT_TRUE(allocator.owns(ptr_default));
    // Check if it's at least 8-byte aligned (very common)
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr_default) % 8, 0);
    allocator.deallocate(ptr_default);
}

TEST(PlainAllocatorTest, Deallocate_Nullptr)
{
    PlainAllocator allocator;
    // Deallocating nullptr should be a safe no-op according to standard C allocator behavior
    // and mimalloc's behavior. It should not crash.
    EXPECT_NO_FATAL_FAILURE(allocator.deallocate(nullptr));
}

// --- Usable Size Tests ---

TEST(PlainAllocatorTest, UsableSize_ValidPointer)
{
    PlainAllocator allocator;
    Size requested_size = 100;
    void* ptr           = allocator.allocate(requested_size);
    EXPECT_NE(nullptr, ptr);

    Size usable_size = allocator.usableSize(ptr);

    // The usable size returned by mi_usable_size should be at least the requested size
    EXPECT_GE(usable_size, requested_size);
    // For a non-zero allocation, the usable size should be greater than 0
    EXPECT_GT(usable_size, 0);

    allocator.deallocate(ptr);
}

TEST(PlainAllocatorTest, UsableSize_Nullptr)
{
    PlainAllocator allocator;
    // Usable size of a nullptr should be 0 according to mimalloc's mi_usable_size behavior
    EXPECT_EQ(allocator.usableSize(nullptr), 0);
}

TEST(PlainAllocatorTest, UsableSize_InvalidPointer) {
     PlainAllocator allocator;
     int dummy;
     void* invalid_ptr_stack = &dummy; // Pointer on the stack
     void* invalid_ptr_heap = malloc(10); // Pointer from a different allocator (if malloc isn't hooked by mimalloc)
     free(invalid_ptr_heap); // Deallocate using original allocator

     EXPECT_EQ(allocator.usableSize(invalid_ptr_stack), 0);
     EXPECT_EQ(allocator.usableSize(invalid_ptr_heap), 0); // Usable size of freed memory
     EXPECT_EQ(allocator.usableSize(reinterpret_cast<void*>(0x12345678)), 0); // Arbitrary pointer
}

// --- Ownership Tests ---

TEST(PlainAllocatorTest, Owns_ValidPointer)
{
    PlainAllocator allocator;
    void* ptr1 = allocator.allocate(50);
    void* ptr2 = allocator.allocate(128);

    EXPECT_NE(nullptr, ptr1);
    EXPECT_NE(nullptr, ptr2);

    // The 'owns' method likely wraps mi_is_valid. A pointer allocated by this
    // mimalloc-based allocator should be considered 'owned'/'valid'.
    EXPECT_TRUE(allocator.owns(ptr1));
    EXPECT_TRUE(allocator.owns(ptr2));

    allocator.deallocate(ptr1);
    allocator.deallocate(ptr2);
}

TEST(PlainAllocatorTest, Owns_Nullptr)
{
    PlainAllocator allocator;
    // A null pointer is not owned by any allocator
    EXPECT_FALSE(allocator.owns(nullptr));
}

TEST(PlainAllocatorTest, Owns_PointerFromDifferentAllocator)
{
    PlainAllocator allocator;
    // Use standard library allocation (assuming it's *not* hooked by mimalloc in this test context)
    void* std_ptr = malloc(64);
    EXPECT_NE(nullptr, std_ptr);

    // PlainAllocator (wrapping mimalloc) should not claim ownership of memory
    // allocated by a different allocator (like stdlib's malloc).
    EXPECT_FALSE(allocator.owns(std_ptr));

    free(std_ptr); // Clean up the stdlib allocation
}

TEST(PlainAllocatorTest, Owns_ArbitraryPointer)
{
    PlainAllocator allocator;
    // An arbitrary pointer should not be considered owned
    EXPECT_FALSE(allocator.owns(reinterpret_cast<void*>(0xDEADBEEF)));
}

TEST(PlainAllocatorTest, Owns_DeallocatedPointer)
{
    PlainAllocator allocator;
    void* ptr = allocator.allocate(32);
    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr)); // Should own before deallocation

    allocator.deallocate(ptr);

    // the ptr point to the old area thought it has been deallocated.
    EXPECT_NE(nullptr, ptr);
    EXPECT_TRUE(allocator.owns(ptr)); // This is the desired logical outcome
}

TEST(PlainAllocatorTest, Owns_OffsetPointer)
{
    PlainAllocator allocator;
    Size requested_size = 100;
    int* ptr           = (int*)allocator.allocate(requested_size);
    EXPECT_NE(nullptr, ptr);

    EXPECT_TRUE(allocator.owns(ptr));
    
    // the size of allocated is bigger than request size
    void* offsetPtr1 = (int*)ptr + 8;
    void* offsetPtr2 = (int*)ptr + 50;
    void* offsetPtr3 = (int*)ptr + 128;
    void* offsetPtr4 = (int*)ptr + 512;
    void* offsetPtr5 = (int*)ptr + 513;
    
    EXPECT_TRUE(allocator.owns(offsetPtr1));
    EXPECT_TRUE(allocator.owns(offsetPtr2));
    EXPECT_TRUE(allocator.owns(offsetPtr3));
    EXPECT_TRUE(allocator.owns(offsetPtr4));
    EXPECT_FALSE(allocator.owns(offsetPtr5));
}

// --- Stress/Multiple Allocation Test ---

TEST(PlainAllocatorTest, AllocateDeallocate_Multiple)
{
    PlainAllocator allocator;
    std::vector<void*> pointers;
    int num_allocations      = 1000;
    Size size_per_allocation = 100;

    // Allocate multiple blocks
    for (int i = 0; i < num_allocations; ++i) {
        void* ptr = allocator.allocate(size_per_allocation);
        EXPECT_NE(nullptr, ptr);
        pointers.push_back(ptr);
    }

    // Deallocate all blocks. Order doesn't strictly matter for a general-purpose allocator.
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
    pointers.clear(); // Clear the vector
}

TEST(PlainAllocatorTest, AllocateDeallocate_MixedSizesAndAlignments)
{
    PlainAllocator allocator;
    std::vector<void*> pointers;
    // Allocate various sizes and alignments
    pointers.push_back(allocator.allocate(100, 8));
    pointers.push_back(allocator.allocate(500, 16));
    pointers.push_back(allocator.allocate(32, 32));
    pointers.push_back(allocator.allocate(1024, 64));
    pointers.push_back(allocator.allocate(17, 0)); // Default alignment

    // Check all allocations were successful
    for (void* ptr : pointers) {
        EXPECT_NE(nullptr, ptr);
    }

    // Deallocate all blocks
    for (void* ptr : pointers) {
        allocator.deallocate(ptr);
    }
    pointers.clear();
}