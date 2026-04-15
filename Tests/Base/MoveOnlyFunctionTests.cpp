/**
 * @File MoveOnlyFunctionTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>

#include "Base/Core/MoveOnlyFunction.hpp"

namespace
{

// =============================================================================
// Basic invocation
// =============================================================================

TEST(MoveOnlyFunctionTest, VoidNoArgs)
{
    bool                          called = false;
    bee::MoveOnlyFunction<void()> fn([&called]() {
        called = true;
    });
    EXPECT_TRUE(fn);
    fn();
    EXPECT_TRUE(called);
}

TEST(MoveOnlyFunctionTest, ReturnValueWithArgs)
{
    bee::MoveOnlyFunction<int(int, int)> fn([](int a, int b) {
        return a + b;
    });
    EXPECT_EQ(fn(3, 4), 7);
}

TEST(MoveOnlyFunctionTest, ConstRefArg)
{
    bee::MoveOnlyFunction<std::size_t(const std::string&)> fn([](const std::string& s) {
        return s.size();
    });
    EXPECT_EQ(fn(std::string("hello")), 5u);
}

TEST(MoveOnlyFunctionTest, FunctionPointer)
{
    bee::MoveOnlyFunction<int(int)> fn(+[](int x) -> int {
        return x * 2;
    });
    EXPECT_EQ(fn(5), 10);
}

// =============================================================================
// Move semantics
// =============================================================================

TEST(MoveOnlyFunctionTest, MoveOnlyCapture)
{
    auto                         ptr = std::make_unique<int>(42);
    bee::MoveOnlyFunction<int()> fn([p = std::move(ptr)]() {
        return *p;
    });
    EXPECT_EQ(fn(), 42);
}

TEST(MoveOnlyFunctionTest, MoveTransfersOwnership)
{
    bee::MoveOnlyFunction<int()> fn([]() {
        return 42;
    });
    auto                         fn2 = std::move(fn);
    EXPECT_FALSE(fn);
    EXPECT_TRUE(fn2);
    EXPECT_EQ(fn2(), 42);
}

TEST(MoveOnlyFunctionTest, MoveAssignment)
{
    bee::MoveOnlyFunction<int()> fn1([]() {
        return 1;
    });
    bee::MoveOnlyFunction<int()> fn2([]() {
        return 2;
    });
    fn1 = std::move(fn2);
    EXPECT_EQ(fn1(), 2);
    EXPECT_FALSE(fn2);
}

// =============================================================================
// SBO vs heap
// =============================================================================

TEST(MoveOnlyFunctionTest, HeapAllocationForLargeCallable)
{
    struct Large
    {
        std::array<std::byte, 256> data{};
        int                        value = 99;
        auto                       operator()() -> int
        {
            return value;
        }
    };
    static_assert(sizeof(Large) > 128, "Must exceed SBO buffer");

    bee::MoveOnlyFunction<int()> fn(Large{});
    EXPECT_EQ(fn(), 99);
}

TEST(MoveOnlyFunctionTest, MoveHeapAllocatedCallable)
{
    struct Large
    {
        std::array<std::byte, 256> data{};
        int                        value = 99;
        auto                       operator()() -> int
        {
            return value;
        }
    };
    static_assert(sizeof(Large) > 128, "Must exceed SBO buffer");

    bee::MoveOnlyFunction<int()> fn1(Large{});
    auto                         fn2 = std::move(fn1);
    EXPECT_FALSE(fn1);
    EXPECT_EQ(fn2(), 99);
}

// =============================================================================
// Empty state
// =============================================================================

TEST(MoveOnlyFunctionTest, DefaultConstructedIsEmpty)
{
    bee::MoveOnlyFunction<void()> fn;
    EXPECT_FALSE(fn);
}

TEST(MoveOnlyFunctionTest, VoidReturnType)
{
    int                              counter = 0;
    bee::MoveOnlyFunction<void(int)> fn([&counter](int x) {
        counter += x;
    });
    fn(3);
    fn(7);
    EXPECT_EQ(counter, 10);
}

} // namespace
