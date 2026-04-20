#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

TEST(BroadcastTests, SameShape)
{
    auto r = compute_broadcast_shape({3}, {3});
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (Shape{3}));
}

TEST(BroadcastTests, TrailingBroadcast)
{
    auto r = compute_broadcast_shape({2, 3}, {3});
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (Shape{2, 3}));
}

TEST(BroadcastTests, TwoDimBroadcast)
{
    auto r = compute_broadcast_shape({2, 1}, {1, 3});
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (Shape{2, 3}));
}

TEST(BroadcastTests, ThreeDimBroadcast)
{
    auto r = compute_broadcast_shape({4, 1, 5}, {3, 5});
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (Shape{4, 3, 5}));
}

TEST(BroadcastTests, IncompatibleShapes)
{
    auto r = compute_broadcast_shape({2, 3}, {4});
    EXPECT_FALSE(r.has_value());
}

TEST(BroadcastTests, EmptyShapeWithNonEmpty)
{
    auto r = compute_broadcast_shape({}, {1, 2, 3});
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (Shape{1, 2, 3}));
}
