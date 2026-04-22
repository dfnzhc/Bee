#include <gtest/gtest.h>
#include <type_traits>

#include "Tensor/Core/DType.hpp"

using namespace bee;

// ── dtype_size 测试 ─────────────────────────────────────────────────────────

TEST(DTypeTests, SizeOfBool)
{
    EXPECT_EQ(dtype_size(DType::Bool), 1u);
}

TEST(DTypeTests, SizeOfU8)
{
    EXPECT_EQ(dtype_size(DType::U8), 1u);
}

TEST(DTypeTests, SizeOfI32)
{
    EXPECT_EQ(dtype_size(DType::I32), 4u);
}

TEST(DTypeTests, SizeOfI64)
{
    EXPECT_EQ(dtype_size(DType::I64), 8u);
}

TEST(DTypeTests, SizeOfF32)
{
    EXPECT_EQ(dtype_size(DType::F32), 4u);
}

TEST(DTypeTests, SizeOfF64)
{
    EXPECT_EQ(dtype_size(DType::F64), 8u);
}

// ── dtype_name 测试 ─────────────────────────────────────────────────────────

TEST(DTypeTests, NamesAreCorrect)
{
    EXPECT_EQ(dtype_name(DType::Bool), "Bool");
    EXPECT_EQ(dtype_name(DType::U8), "U8");
    EXPECT_EQ(dtype_name(DType::I32), "I32");
    EXPECT_EQ(dtype_name(DType::I64), "I64");
    EXPECT_EQ(dtype_name(DType::F32), "F32");
    EXPECT_EQ(dtype_name(DType::F64), "F64");
}

// ── 编译期双向映射测试 ──────────────────────────────────────────────────────

TEST(DTypeTests, DtypeVFloat)
{
    EXPECT_EQ(dtype_v<float>, DType::F32);
}

TEST(DTypeTests, DtypeVDouble)
{
    EXPECT_EQ(dtype_v<double>, DType::F64);
}

TEST(DTypeTests, DtypeVInt32)
{
    EXPECT_EQ(dtype_v<int32_t>, DType::I32);
}

TEST(DTypeTests, DtypeVInt64)
{
    EXPECT_EQ(dtype_v<int64_t>, DType::I64);
}

TEST(DTypeTests, DtypeVBool)
{
    EXPECT_EQ(dtype_v<bool>, DType::Bool);
}

TEST(DTypeTests, DtypeVUint8)
{
    EXPECT_EQ(dtype_v<uint8_t>, DType::U8);
}

TEST(DTypeTests, DtypeCppTF32IsFloat)
{
    constexpr bool ok = std::is_same_v<dtype_cpp_t<DType::F32>, float>;
    EXPECT_TRUE(ok);
}

TEST(DTypeTests, DtypeCppTI64IsInt64)
{
    constexpr bool ok = std::is_same_v<dtype_cpp_t<DType::I64>, int64_t>;
    EXPECT_TRUE(ok);
}

TEST(DTypeTests, DtypeCppTBoolIsBool)
{
    constexpr bool ok = std::is_same_v<dtype_cpp_t<DType::Bool>, bool>;
    EXPECT_TRUE(ok);
}
