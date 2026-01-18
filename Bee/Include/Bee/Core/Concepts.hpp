/**
 * @File Concepts.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/23
 * @Brief 定义通用的 Concepts
 */

#pragma once

#include <concepts>
#include "Defines.hpp"

namespace bee
{
template <class>
constexpr bool AlwaysFalse = false;

// clang-format off
template <typename T> concept BoolType = std::is_same_v<bool, T>;
template <typename T> concept U32Type  = std::is_same_v<u32, T>;
template <typename T> concept U64Type  = std::is_same_v<u64, T>;
template <typename T> concept F32Type  = std::is_same_v<f32, T>;
template <typename T> concept F64Type  = std::is_same_v<f64, T>;

template <typename T> concept SignedType   = std::is_signed_v<T>;
template <typename T> concept UnsignedType = std::is_unsigned_v<T>;
template <typename T> concept IntegralType = std::is_integral_v<T>;
template <typename T> concept FloatType    = std::is_floating_point_v<T>;
template <typename T> concept ArithType    = std::is_arithmetic_v<T>;
// clang-format on

template <ArithType T>
using MapFloatType = std::conditional_t<sizeof(T) <= 4, f32, f64>;

template <typename T, typename... Ts>
using CommonFloatType = MapFloatType<std::common_type_t<T, Ts...>>;

// 用于数值类型转换
template <ArithType T>
constexpr T To(ArithType auto f) noexcept
    requires std::is_nothrow_convertible_v<decltype(f), T>
{
    return static_cast<T>(f);
}

template <typename From, typename To>
concept ConvertibleTo = std::is_convertible_v<From, To>;

template <typename T, typename... Ts>
concept AllConvertibleTo = (ConvertibleTo<T, Ts> && ...);

// ==================== Tuple/Vector Concepts ====================

// 具有固定维度的类型
template <typename T>
concept FixedDimension = requires {
    { T::Dimension } -> std::convertible_to<int>;
    requires std::same_as<const int&, decltype(T::Dimension)>;
};

// 具有 value_type 定义
template <typename T>
concept HasValueType = requires {
    typename T::value_type;
};

// 可索引类型 - 必须支持 operator[]
template <typename T, int N = -1>
concept Indexable = HasValueType<T> && requires(T t, int i) {
    { t[i] } -> std::convertible_to<typename T::value_type>;
    { t[0] } -> std::same_as<typename T::value_type&>;
    { std::as_const(t)[0] } -> std::same_as<const typename T::value_type&>;
};

// 可变可索引类型 - 必须支持可修改的 operator[]
template <typename T, int N = -1>
concept MutableIndexable = Indexable<T, N> && requires(T t, typename T::value_type v, int i) {
    { t[i] = v } -> std::same_as<typename T::value_type&>;
};

// 具有公共 x, y 成员（用于 2D 类型）
template <typename T>
concept HasPublicXY = HasValueType<T> && requires(T t) {
    { t.x } -> std::convertible_to<typename T::value_type>;
    { t.y } -> std::convertible_to<typename T::value_type>;
};

// 具有公共 x, y, z 成员（用于 3D 类型）
template <typename T>
concept HasPublicXYZ = HasValueType<T> && requires(T t) {
    { t.x } -> std::convertible_to<typename T::value_type>;
    { t.y } -> std::convertible_to<typename T::value_type>;
    { t.z } -> std::convertible_to<typename T::value_type>;
};

// 具有公共 x, y, z, w 成员（用于 4D 类型）
template <typename T>
concept HasPublicXYZW = HasPublicXYZ<T> && requires(T t) {
    { t.w } -> std::convertible_to<typename T::value_type>;
};

// CUDA 兼容性类型
template <typename T>
concept CUDACompatible = std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;

} // namespace bee
