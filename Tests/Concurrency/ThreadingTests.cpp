/**
 * @File BaseUtilsTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string_view>

#include "Base/Bit.hpp"
#include "Concurrency/Threading.hpp"

TEST(BaseThreadingTests, ThreadPauseIsCallable)
{
    for (std::uint32_t i = 0; i < 256; ++i) {
        bee::ThreadPauseRelaxed();
    }
    SUCCEED();
}

TEST(BaseThreadingTests, ThreadPauseVariantsAndContentionHelpersAreCallable)
{
    bee::ThreadYield();
    bee::ThreadPauseRelaxed();
    
    bee::DefaultSpinPolicy::wait(1);
    SUCCEED();
    
    bee::ThroughputDefaultSpinPolicy::wait(1);
    SUCCEED();
}

TEST(BaseThreadingTests, HardwareThreadCountAndThreadIdHashAreAvailable)
{
    EXPECT_GE(bee::HardwareThreadCount(), 1u);
    const auto id_hash = bee::ThreadIdHash();
    EXPECT_EQ(id_hash, bee::ThreadIdHash());
}

TEST(BaseThreadingTests, SleepUtilitiesAreCallable)
{
    const auto begin = std::chrono::steady_clock::now();
    bee::SleepForNanos(1000);
    bee::SleepForMicros(1);
    const auto end = std::chrono::steady_clock::now();
    EXPECT_GE(end, begin);
}

TEST(BaseThreadingTests, SetCurrentThreadNameAcceptsNonEmptyName)
{
    EXPECT_FALSE(bee::SetCurrentThreadName(std::string_view{}));
    SUCCEED() << bee::SetCurrentThreadName("BaseThreadingTests");
}
