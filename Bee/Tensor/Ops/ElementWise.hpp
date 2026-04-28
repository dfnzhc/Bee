#pragma once

// 元素级算子自由函数声明。
//
// 二元算子支持 NumPy 风格广播，输出 shape 为两侧 shape 的广播结果；
// dtype 与 device 必须一致。CPU 路径包含连续 fast-path 与 stride slow-path；
// CUDA 路径会在必要时先物化连续输入，再调用设备端连续 kernel。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// 返回 a + b 的新张量。
[[nodiscard]] auto add(const Tensor& a, const Tensor& b) -> Result<Tensor>;
// 返回 a - b 的新张量。
[[nodiscard]] auto sub(const Tensor& a, const Tensor& b) -> Result<Tensor>;
// 返回 a * b 的新张量。
[[nodiscard]] auto mul(const Tensor& a, const Tensor& b) -> Result<Tensor>;
// 返回 a / b 的新张量；除零语义遵循底层 dtype 的 C++/CUDA 行为。
[[nodiscard]] auto div(const Tensor& a, const Tensor& b) -> Result<Tensor>;

// 返回 -a 的新张量。
[[nodiscard]] auto neg(const Tensor& a) -> Result<Tensor>;
// 返回 abs(a) 的新张量。
[[nodiscard]] auto abs(const Tensor& a) -> Result<Tensor>;
// 返回 sqrt(a) 的新张量。
[[nodiscard]] auto sqrt(const Tensor& a) -> Result<Tensor>;
// 返回 exp(a) 的新张量。
[[nodiscard]] auto exp(const Tensor& a) -> Result<Tensor>;
// 返回 log(a) 的新张量。
[[nodiscard]] auto log(const Tensor& a) -> Result<Tensor>;
// 返回 relu(a) 的新张量。
[[nodiscard]] auto relu(const Tensor& a) -> Result<Tensor>;
// 返回 sigmoid(a) 的新张量。
[[nodiscard]] auto sigmoid(const Tensor& a) -> Result<Tensor>;

// 将 dst 与 src 做广播兼容的原地加法；结果写回 dst。
[[nodiscard]] auto add_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
// 将 dst 与 src 做广播兼容的原地减法；结果写回 dst。
[[nodiscard]] auto sub_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
// 将 dst 与 src 做广播兼容的原地乘法；结果写回 dst。
[[nodiscard]] auto mul_inplace(Tensor& dst, const Tensor& src) -> Result<void>;
// 将 dst 与 src 做广播兼容的原地除法；结果写回 dst。
[[nodiscard]] auto div_inplace(Tensor& dst, const Tensor& src) -> Result<void>;

// 对 dst 原地取负。
[[nodiscard]] auto neg_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地取绝对值。
[[nodiscard]] auto abs_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地求平方根。
[[nodiscard]] auto sqrt_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地求自然指数。
[[nodiscard]] auto exp_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地求自然对数。
[[nodiscard]] auto log_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地应用 ReLU。
[[nodiscard]] auto relu_inplace(Tensor& dst) -> Result<void>;
// 对 dst 原地应用 Sigmoid。
[[nodiscard]] auto sigmoid_inplace(Tensor& dst) -> Result<void>;

} // namespace bee
