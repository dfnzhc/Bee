#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace bee
{

// 元素数据类型枚举
//
// MVP 可计算类型：Bool/U8/I32/I64/F32/F64。
// 扩展类型（F16/BF16/FP8E4M3/FP8E5M2/FP4）仅作为不透明存储占位：
//  - 仅定义字节大小（F16/BF16=2；FP8*=1；FP4=1，按打包 2 元素/字节语义暂以 1 字节对齐表示）。
//  - CPU 算子遇到这些类型统一返回 NotImplemented 错误（不参与运算）。
//  - 真正的计算实现后续在 CUDA 组件内接入（CUTLASS/wmma/tcgen05 路径）。
enum class DType : uint8_t
{
    Bool,
    U8,
    I32,
    I64,
    F32,
    F64,
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

// 返回数据类型名称字符串
[[nodiscard]] constexpr auto dtype_name(DType dt) noexcept -> std::string_view
{
    switch (dt) {
    case DType::Bool: return "Bool";
    case DType::U8: return "U8";
    case DType::I32: return "I32";
    case DType::I64: return "I64";
    case DType::F32: return "F32";
    case DType::F64: return "F64";
    case DType::F16: return "F16";
    case DType::BF16: return "BF16";
    case DType::FP8E4M3: return "FP8E4M3";
    case DType::FP8E5M2: return "FP8E5M2";
    case DType::FP4: return "FP4";
    default: return "Unknown";
    }
}

// 判断 DType 是否为 CPU 可计算类型（Bool/U8/I32/I64/F32/F64）。
[[nodiscard]] constexpr auto dtype_is_cpu_computable(DType dt) noexcept -> bool
{
    switch (dt) {
    case DType::Bool:
    case DType::U8:
    case DType::I32:
    case DType::I64:
    case DType::F32:
    case DType::F64:
        return true;
    default:
        return false;
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

} // namespace bee
