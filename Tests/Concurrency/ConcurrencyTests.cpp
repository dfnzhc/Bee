#include <gtest/gtest.h>

#include "Concurrency/Concurrency.hpp"

TEST(ConcurrencyComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(bee::concurrency_name(), "Concurrency");
}

TEST(ConcurrencyComponentTests, LinksAndUsesBaseComponent)
{
    EXPECT_EQ(bee::concurrency_base_name(), "Base");
}
