#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace bee
{

// 元素数据类型枚举
//
// MVP CPU 可计算类型：Bool/U8/I8/I32/I64/F32/F64。
// CUDA 可计算类型（额外包含）：F16/BF16。
// 低精度存储类型（CPU 仅支持存储与 F32 互转）：F16/BF16。
// 占位类型（暂不参与任何计算）：FP8E4M3/FP8E5M2/FP4。
enum class DType : uint8_t
{
    Bool,
    U8,
    I32,
    I64,
    F32,
    F64,
    I8,
    // —— 扩展占位（CPU 上不可计算）——
    F16,
    BF16,
    FP8E4M3,
    FP8E5M2,
    FP4,
};

// 返回数据类型对应的字节数
[[nodiscard]] constexpr auto dtype_size(DType dt) noexcept -> std::size_t
{
    switch (dt) {
    case DType::Bool: return 1;
    case DType::U8: return 1;
    case DType::I8: return 1;
    case DType::I32: return 4;
    case DType::I64: return 8;
    case DType::F32: return 4;
    case DType::F64: return 8;
    case DType::F16: return 2;
    case DType::BF16: return 2;
    case DType::FP8E4M3: return 1;
    case DType::FP8E5M2: return 1;
    // FP4：2 元素/字节打包。单元素语义尺寸无意义，按 1 字节返回仅用于标量占位；
    // 真正的张量内存分配应按 ceil(numel/2) 字节；CPU 端目前不支持创建 FP4 张量。
    case DType::FP4: return 1;
    default: return 0;
    }
}

// 判断 DType 是否为 CPU 可计算类型（Bool/U8/I32/I64/F32/F64）。
[[nodiscard]] constexpr auto dtype_is_cpu_computable(DType dt) noexcept -> bool
{
    switch (dt) {
    case DType::Bool:
    case DType::U8:
    case DType::I8:
    case DType::I32:
    case DType::I64:
    case DType::F32:
    case DType::F64: return true;
    default: return false;
    }
}

// 编译期双向映射：DType → C++ 原生类型
template <DType D>
struct DTypeToCpp;

template <>
struct DTypeToCpp<DType::Bool>
{
    using type = bool;
};

template <>
struct DTypeToCpp<DType::U8>
{
    using type = uint8_t;
};

template <>
struct DTypeToCpp<DType::I8>
{
    using type = int8_t;
};

template <>
struct DTypeToCpp<DType::I32>
{
    using type = int32_t;
};

template <>
struct DTypeToCpp<DType::I64>
{
    using type = int64_t;
};

template <>
struct DTypeToCpp<DType::F32>
{
    using type = float;
};

template <>
struct DTypeToCpp<DType::F64>
{
    using type = double;
};

// 编译期双向映射：C++ 原生类型 → DType
template <typename T>
struct CppToDType;

template <>
struct CppToDType<bool>
{
    static constexpr DType value = DType::Bool;
};

template <>
struct CppToDType<uint8_t>
{
    static constexpr DType value = DType::U8;
};

template <>
struct CppToDType<int8_t>
{
    static constexpr DType value = DType::I8;
};

template <>
struct CppToDType<int32_t>
{
    static constexpr DType value = DType::I32;
};

template <>
struct CppToDType<int64_t>
{
    static constexpr DType value = DType::I64;
};

template <>
struct CppToDType<float>
{
    static constexpr DType value = DType::F32;
};

template <>
struct CppToDType<double>
{
    static constexpr DType value = DType::F64;
};

// 便捷别名：DType → C++ 类型
template <DType D>
using dtype_cpp_t = typename DTypeToCpp<D>::type;

// 便捷变量模板：C++ 类型 → DType
template <typename T>
inline constexpr DType dtype_v = CppToDType<T>::value;

// ─── 算子类型枚举（用于确定累加/提升规则）────────────────────────────────────
enum class DTypeOpKind : uint8_t
{
    ElementWise, // 元素级：保持输入类型
    Reduce,      // 规约：低精度累加至 F32
    Matmul,      // 矩阵乘：低精度累加至 F32
    Norm,        // 归一化：低精度累加至 F32
    Softmax,     // Softmax：低精度累加至 F32
};

// 判断 DType 是否为 CUDA 可计算类型（在 CPU 可计算类型基础上增加 F16/BF16）。
[[nodiscard]] constexpr auto dtype_is_cuda_computable(DType dt) noexcept -> bool
{
    switch (dt) {
    case DType::Bool:
    case DType::U8:
    case DType::I8:
    case DType::I32:
    case DType::I64:
    case DType::F32:
    case DType::F64:
    case DType::F16:
    case DType::BF16: return true;
    default: return false;
    }
}

// 返回给定算子类型下的累加/输出类型。
// 规则：F16/BF16 在非 ElementWise 算子中累加至 F32；其余类型保持不变。
[[nodiscard]] constexpr auto dtype_accumulate_type(DTypeOpKind op, DType dt) noexcept -> DType
{
    if ((dt == DType::F16 || dt == DType::BF16) && op != DTypeOpKind::ElementWise)
        return DType::F32;
    return dt;
}

// 返回两操作数类型在给定算子下的提升结果。
// 规则（优先级由高到低）：
//   1. 相同类型 → 走 accumulate 规则
//   2. F64 一方存在 → F64
//   3. F32 一方存在，或 F16/BF16 混合 → F32
//   4. I64 优先于 I32/U8/I8
//   5. I32 优先于 U8/I8
//   6. 其余取左操作数类型
[[nodiscard]] constexpr auto promote_types(DType lhs, DType rhs, DTypeOpKind op) noexcept -> DType
{
    if (lhs == rhs)
        return dtype_accumulate_type(op, lhs);
    if (lhs == DType::F64 || rhs == DType::F64)
        return DType::F64;
    if (lhs == DType::F32 || rhs == DType::F32)
        return DType::F32;
    if ((lhs == DType::F16 || lhs == DType::BF16) && (rhs == DType::F16 || rhs == DType::BF16))
        return DType::F32;
    if (lhs == DType::F16 || rhs == DType::F16 || lhs == DType::BF16 || rhs == DType::BF16)
        return DType::F32;
    if (lhs == DType::I64 || rhs == DType::I64)
        return DType::I64;
    if (lhs == DType::I32 || rhs == DType::I32)
        return DType::I32;
    return lhs;
}

} // namespace bee
