#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace bee
{

// 元素数据类型枚举
enum class DType : uint8_t
{
    Bool,
    U8,
    I32,
    I64,
    F32,
    F64,
};

// 返回数据类型对应的字节数
[[nodiscard]] constexpr auto dtype_size(DType dt) noexcept -> std::size_t
{
    switch (dt) {
    case DType::Bool: return 1;
    case DType::U8:   return 1;
    case DType::I32:  return 4;
    case DType::I64:  return 8;
    case DType::F32:  return 4;
    case DType::F64:  return 8;
    default:          return 0;
    }
}

// 返回数据类型名称字符串
[[nodiscard]] constexpr auto dtype_name(DType dt) noexcept -> std::string_view
{
    switch (dt) {
    case DType::Bool: return "Bool";
    case DType::U8:   return "U8";
    case DType::I32:  return "I32";
    case DType::I64:  return "I64";
    case DType::F32:  return "F32";
    case DType::F64:  return "F64";
    default:          return "Unknown";
    }
}

// 编译期双向映射：DType → C++ 原生类型
template <DType D>
struct DTypeToCpp;

template <>
struct DTypeToCpp<DType::Bool> { using type = bool; };

template <>
struct DTypeToCpp<DType::U8>   { using type = uint8_t; };

template <>
struct DTypeToCpp<DType::I32>  { using type = int32_t; };

template <>
struct DTypeToCpp<DType::I64>  { using type = int64_t; };

template <>
struct DTypeToCpp<DType::F32>  { using type = float; };

template <>
struct DTypeToCpp<DType::F64>  { using type = double; };

// 编译期双向映射：C++ 原生类型 → DType
template <typename T>
struct CppToDType;

template <>
struct CppToDType<bool>     { static constexpr DType value = DType::Bool; };

template <>
struct CppToDType<uint8_t>  { static constexpr DType value = DType::U8; };

template <>
struct CppToDType<int32_t>  { static constexpr DType value = DType::I32; };

template <>
struct CppToDType<int64_t>  { static constexpr DType value = DType::I64; };

template <>
struct CppToDType<float>    { static constexpr DType value = DType::F32; };

template <>
struct CppToDType<double>   { static constexpr DType value = DType::F64; };

// 便捷别名：DType → C++ 类型
template <DType D>
using dtype_cpp_t = typename DTypeToCpp<D>::type;

// 便捷变量模板：C++ 类型 → DType
template <typename T>
inline constexpr DType dtype_v = CppToDType<T>::value;

} // namespace bee
