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

#include "Concurrency/Threading.hpp"

TEST(BaseThreadingTests, ThreadPauseIsCallable)
{
    for (std::uint32_t i = 0; i < 256; ++i) {
        bee::ThreadPause(i);
    }
    SUCCEED();
}

TEST(BaseThreadingTests, ThreadPauseVariantsAndContentionHelpersAreCallable)
{
    bee::ThreadYield();
    bee::ThreadPauseRelaxed();
    bee::ThreadPauseWithYield(63, 0x3Fu);

    std::uint32_t spin_count = 0;
    bee::OnTryContention(spin_count, 0x01u);
    EXPECT_EQ(spin_count, 1u);

    bee::AdaptiveSpinWait(spin_count, 0x03u, 8u, 16u);
    EXPECT_EQ(spin_count, 2u);
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
