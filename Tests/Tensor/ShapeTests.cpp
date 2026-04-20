#include <gtest/gtest.h>

#include "Tensor/Core/Shape.hpp"

using namespace bee;

// ── numel 测试 ──────────────────────────────────────────────────────────────

TEST(ShapeTests, NUmel3D)
{
    EXPECT_EQ(numel({2, 3, 4}), 24);
}

TEST(ShapeTests, NumelScalar)
{
    EXPECT_EQ(numel({}), 1);
}

TEST(ShapeTests, Numel1D)
{
    EXPECT_EQ(numel({5}), 5);
}

TEST(ShapeTests, NumelWithOneDim)
{
    EXPECT_EQ(numel({1, 6}), 6);
}

// ── compute_contiguous_strides 测试 ─────────────────────────────────────────

TEST(ShapeTests, ContiguousStrides3D)
{
    const Strides expected = {12, 4, 1};
    EXPECT_EQ(compute_contiguous_strides({2, 3, 4}), expected);
}

TEST(ShapeTests, ContiguousStridesScalar)
{
    EXPECT_TRUE(compute_contiguous_strides({}).empty());
}

TEST(ShapeTests, ContiguousStrides1D)
{
    const Strides expected = {1};
    EXPECT_EQ(compute_contiguous_strides({7}), expected);
}

TEST(ShapeTests, ContiguousStrides2D)
{
    const Strides expected = {4, 1};
    EXPECT_EQ(compute_contiguous_strides({3, 4}), expected);
}

// ── is_contiguous 测试 ──────────────────────────────────────────────────────

TEST(ShapeTests, IsContiguousTrue)
{
    EXPECT_TRUE(is_contiguous({2, 3, 4}, {12, 4, 1}));
}

TEST(ShapeTests, IsContiguousFalse)
{
    // 步长不连续
    EXPECT_FALSE(is_contiguous({2, 3, 4}, {12, 4, 2}));
}

TEST(ShapeTests, IsContiguousScalar)
{
    EXPECT_TRUE(is_contiguous({}, {}));
}

TEST(ShapeTests, IsContiguousSize1DimAnyStride)
{
    // size==1 的维度步长任意，仍视为连续
    EXPECT_TRUE(is_contiguous({2, 1, 4}, {4, 999, 1}));
}

TEST(ShapeTests, IsContiguousLeadingSize1)
{
    EXPECT_TRUE(is_contiguous({1, 3, 4}, {100, 4, 1}));
}

TEST(ShapeTests, IsNotContiguousWrongInnerStride)
{
    // 最右侧维度步长不为 1
    EXPECT_FALSE(is_contiguous({3, 4}, {8, 2}));
}

// ── shapes_equal 测试 ────────────────────────────────────────────────────────

TEST(ShapeTests, ShapesEqualTrue)
{
    EXPECT_TRUE(shapes_equal({2, 3, 4}, {2, 3, 4}));
}

TEST(ShapeTests, ShapesEqualFalse)
{
    EXPECT_FALSE(shapes_equal({2, 3}, {2, 4}));
}

TEST(ShapeTests, ShapesEqualDifferentRank)
{
    EXPECT_FALSE(shapes_equal({2, 3}, {2, 3, 1}));
}

TEST(ShapeTests, ShapesEqualEmpty)
{
    EXPECT_TRUE(shapes_equal({}, {}));
}
