#include <gtest/gtest.h>
#include <type_traits>

#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Tensor.hpp"

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

// ── enum_to_name 测试 ─────────────────────────────────────────────────────────

TEST(DTypeTests, NamesAreCorrect)
{
    EXPECT_EQ(enum_to_name(DType::Bool), "Bool");
    EXPECT_EQ(enum_to_name(DType::U8), "U8");
    EXPECT_EQ(enum_to_name(DType::I32), "I32");
    EXPECT_EQ(enum_to_name(DType::I64), "I64");
    EXPECT_EQ(enum_to_name(DType::F32), "F32");
    EXPECT_EQ(enum_to_name(DType::F64), "F64");
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

// ── M9：扩展 dtype 占位（F16/BF16/FP8*/FP4）──────────────────────────────────

TEST(DTypeTests, ExtendedDtypeSizes)
{
    EXPECT_EQ(dtype_size(DType::F16), 2u);
    EXPECT_EQ(dtype_size(DType::BF16), 2u);
    EXPECT_EQ(dtype_size(DType::FP8E4M3), 1u);
    EXPECT_EQ(dtype_size(DType::FP8E5M2), 1u);
    EXPECT_EQ(dtype_size(DType::FP4), 1u);
}

TEST(DTypeTests, ExtendedDtypeNames)
{
    EXPECT_EQ(enum_to_name(DType::F16), "F16");
    EXPECT_EQ(enum_to_name(DType::BF16), "BF16");
    EXPECT_EQ(enum_to_name(DType::FP8E4M3), "FP8E4M3");
    EXPECT_EQ(enum_to_name(DType::FP8E5M2), "FP8E5M2");
    EXPECT_EQ(enum_to_name(DType::FP4), "FP4");
}

TEST(DTypeTests, ExtendedDtypeNotCpuComputable)
{
    EXPECT_FALSE(dtype_is_cpu_computable(DType::F16));
    EXPECT_FALSE(dtype_is_cpu_computable(DType::BF16));
    EXPECT_FALSE(dtype_is_cpu_computable(DType::FP8E4M3));
    EXPECT_FALSE(dtype_is_cpu_computable(DType::FP8E5M2));
    EXPECT_FALSE(dtype_is_cpu_computable(DType::FP4));
    EXPECT_TRUE(dtype_is_cpu_computable(DType::F32));
    EXPECT_TRUE(dtype_is_cpu_computable(DType::I64));
}

TEST(DTypeTests, ExtendedDtypeEmptyAllocOk)
{
    // empty 允许扩展 dtype：只分配内存，不做初始化。
    auto t = Tensor::empty({8}, DType::F16, Device::CPU);
    ASSERT_TRUE(t);
    EXPECT_EQ(t->dtype(), DType::F16);
    EXPECT_EQ(t->numel(), 8);
}

TEST(DTypeTests, ExtendedDtypeFullReturnsNotImpl)
{
    // full/ones 依赖标量填充：扩展 dtype 应返回 NotImplemented。
    auto r = Tensor::full({4}, DType::BF16, 1.0, Device::CPU);
    ASSERT_FALSE(r);
}
